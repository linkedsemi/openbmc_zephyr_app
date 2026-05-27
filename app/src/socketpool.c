
/* SPDX-License-Identifier: Apache-2.0 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

/* dbus-broker headers */
#include "broker.h"
#include "bus/peer.h"

LOG_MODULE_REGISTER(SOCKETPOOL, LOG_LEVEL_INF);

/*
 * Socketpool configuration
 */
#ifdef CONFIG_DBUS_BROKER_SOCKETPOOL
#define SOCKETPOOL_MAX_PAIRS  CONFIG_DBUS_BROKER_SOCKETPOOL_SIZE
#define SOCKETPOOL_BUFFER_SIZE CONFIG_DBUS_BROKER_SOCKETPOOL_BUFFER_SIZE
#else
#define SOCKETPOOL_MAX_PAIRS  1
#define SOCKETPOOL_BUFFER_SIZE 1
#endif

/*
 * Socketpool entry
 */
struct socketpool_entry {
    int broker_fd;
    int client_fd;
    bool in_use;
    struct k_sem sem;
};

/*
 * Socketpool global state
 */
struct socketpool {
    struct k_mutex lock;
    struct socketpool_entry entries[SOCKETPOOL_MAX_PAIRS];
    int total_pairs;
};

static struct socketpool socketpool = {
    .lock = Z_MUTEX_INITIALIZER(socketpool.lock),
    .total_pairs = 0,
};

/*
 * Create a single socketpair and configure it
 */
static int socketpool_create_pair(struct socketpool_entry *entry)
{
    int sv[2];
    int r;

    r = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    if (r < 0) {
        LOG_ERR("[Socketpool] Failed to create socketpair: %d (errno: %d)", r, errno);
        return -errno;
    }

    entry->broker_fd = sv[0];
    entry->client_fd = sv[1];

    /* Set both ends to non-blocking */
    int flags;
    flags = fcntl(entry->broker_fd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(entry->broker_fd, F_SETFL, flags | O_NONBLOCK);

    flags = fcntl(entry->client_fd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(entry->client_fd, F_SETFL, flags | O_NONBLOCK);

    socketpool.total_pairs++;

    return 0;
}

/*
 * Initialize socketpool - lazy allocation, no socketpairs created yet.
 * Socketpairs are created on-demand in socketpool_allocate().
 */
int socketpool_init(void)
{
    int i;

    LOG_INF("[Socketpool] Initializing socketpool pool size=%d (lazy allocation)...",
            SOCKETPOOL_MAX_PAIRS);

    for (i = 0; i < SOCKETPOOL_MAX_PAIRS; i++) {
        struct socketpool_entry *entry = &socketpool.entries[i];
        entry->broker_fd = -1;
        entry->client_fd = -1;
        entry->in_use = false;
        k_sem_init(&entry->sem, 1, 1);
    }

    LOG_INF("[Socketpool] Socketpool initialized: 0/%d pairs created (on-demand)",
            SOCKETPOOL_MAX_PAIRS);

    return 0;
}

/*
 * Allocate a socketpair from the pool (lazy creation)
 * Returns: 0 on success, negative errno on failure
 * On success, broker_fd and client_fd are set to the allocated fds
 */
int socketpool_allocate(int *broker_fd, int *client_fd)
{
    int i;
    struct socketpool_entry *entry = NULL;
    int r;

    if (!broker_fd || !client_fd) {
        LOG_ERR("[Socketpool] Invalid parameters");
        return -EINVAL;
    }

    k_mutex_lock(&socketpool.lock, K_FOREVER);

    for (i = 0; i < SOCKETPOOL_MAX_PAIRS; i++) {
        if (!socketpool.entries[i].in_use && socketpool.entries[i].broker_fd == -1) {
            entry = &socketpool.entries[i];
            r = socketpool_create_pair(entry);
            if (r < 0) {
                k_mutex_unlock(&socketpool.lock);
                return r;
            }
            break;
        }
    }

    if (!entry) {
        LOG_ERR("[Socketpool] No available socketpairs in pool (max=%d, created=%d)",
                SOCKETPOOL_MAX_PAIRS, socketpool.total_pairs);
        k_mutex_unlock(&socketpool.lock);
        return -EAGAIN;
    }

    entry->in_use = true;

    *broker_fd = entry->broker_fd;
    *client_fd = entry->client_fd;

    k_mutex_unlock(&socketpool.lock);

    LOG_DBG("[Socketpool] Allocated socketpair: broker_fd=%d, client_fd=%d (%d/%d created)",
            *broker_fd, *client_fd, socketpool.total_pairs, SOCKETPOOL_MAX_PAIRS);

    return 0;
}

/*
 * Free a socketpair back to the pool (closes fds, releases pipe buffer memory)
 * Returns: 0 on success, negative errno on failure
 */
int socketpool_free(int broker_fd, int client_fd)
{
    int i;
    struct socketpool_entry *entry = NULL;

    k_mutex_lock(&socketpool.lock, K_FOREVER);

    for (i = 0; i < socketpool.total_pairs; i++) {
        if (socketpool.entries[i].broker_fd == broker_fd &&
            socketpool.entries[i].client_fd == client_fd) {
            entry = &socketpool.entries[i];
            break;
        }
    }

    if (!entry) {
        LOG_ERR("[Socketpool] Socketpair not found: broker_fd=%d, client_fd=%d",
                broker_fd, client_fd);
        k_mutex_unlock(&socketpool.lock);
        return -ENOENT;
    }

    if (!entry->in_use) {
        LOG_WRN("[Socketpool] Socketpair already free: broker_fd=%d, client_fd=%d",
                broker_fd, client_fd);
        k_mutex_unlock(&socketpool.lock);
        return 0;
    }

    /* Close fds to release kernel pipe buffers back to heap */
    if (entry->broker_fd >= 0)
        close(entry->broker_fd);
    if (entry->client_fd >= 0 && entry->client_fd != entry->broker_fd)
        close(entry->client_fd);

    entry->broker_fd = -1;
    entry->client_fd = -1;
    entry->in_use = false;
    socketpool.total_pairs--;

    k_mutex_unlock(&socketpool.lock);

    LOG_DBG("[Socketpool] Freed socketpair: closed fds (%d/%d created)",
            socketpool.total_pairs, SOCKETPOOL_MAX_PAIRS);

    return 0;
}

/*
 * Add a peer to the broker using a socketpair from the pool
 * This function is called when a client requests a connection
 * Returns: 0 on success, negative errno on failure
 */
int socketpool_add_peer_to_broker(Broker *broker, int broker_fd)
{
    Peer *peer = NULL;
    int r;
    char guid[16] = "0123456789abcdef";

    if (!broker) {
        LOG_ERR("[Socketpool] Invalid broker");
        return -EINVAL;
    }

    LOG_DBG("[Socketpool] Creating peer for broker_fd=%d", broker_fd);

    /* Create a new peer with the broker_fd */
    r = peer_new_with_fd(&peer, &broker->bus, NULL, guid,
                        &broker->dispatcher, broker_fd);
    if (r < 0) {
        LOG_ERR("[Socketpool] Failed to create peer: %d", r);
        return r;
    }

    if (peer->policy != NULL) {
        LOG_WRN("[Socketpool] Unexpected non-NULL policy on Zephyr");
    }

    /* Do NOT register the peer here - let the peer register itself via Hello message */
    /* peer_register(peer); */

    LOG_DBG("[Socketpool] Peer created: fd=%d, peer_id=%d (will register via Hello)", broker_fd, peer->id);

    /* Spawn the peer (open the connection) */
    r = peer_spawn(peer);
    if (r < 0) {
        LOG_ERR("[Socketpool] Failed to spawn peer: %d", r);
        peer_unregister(peer);
        peer_free(peer);
        return r;
    }

    LOG_INF("[Socketpool] Peer spawned successfully: fd=%d, connection socket_file user_mask=0x%x",
            broker_fd, peer->connection.socket_file.user_mask);

    /* Peer is now owned by the broker, don't free it */
    return 0;
}

void socketpool_wake_broker(Broker *broker)
{
    if (broker) {
        char byte = 1;
        write(broker->dispatcher.terminate_pipe[1], &byte, 1);
    }
}
