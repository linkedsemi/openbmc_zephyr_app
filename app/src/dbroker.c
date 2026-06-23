/* SPDX-License-Identifier: Apache-2.0 */

/* Define basu config macros before including any basu headers */
#ifndef SIZEOF_PID_T
#define SIZEOF_PID_T 4
#endif

#ifndef SIZEOF_UID_T
#define SIZEOF_UID_T 4
#endif

#ifndef SIZEOF_GID_T
#define SIZEOF_GID_T 4
#endif

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <zephyr/net/net_ip.h>
#include <stdio.h>
#include <soc.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "task_def.hpp"

/* dbus-broker headers */
#include "broker.h"
#include "systemd/sd-bus.h"
#include "util/string.h"

/* D-Bus broker subsystem */
#include "dbus_broker.h"

LOG_MODULE_REGISTER(DBROKER, LOG_LEVEL_DBG);

#define DBUS_BROKER_SOCK_PATH "/tmp/dbus-broker"

/*
 * Global variables for broker communication
 */
int g_controller_fds[2] = { -1, -1 };
struct deployment_state {
    struct k_mutex lock;
    bool broker_ready;
    int listener_fd;
    bool service_provider_ready;
};
struct deployment_state deploy_state = {
    .lock = Z_MUTEX_INITIALIZER(deploy_state.lock),
    .broker_ready = false,
    .listener_fd = -1,
    .service_provider_ready = false
};
extern Broker *g_broker;

/* ================================================================== */
/* Listener socket (AF_UNIX)                                           */
/* ================================================================== */
static int create_listener_socket(void)
{
    int fd;
    int flags;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        LOG_ERR("Failed to create AF_UNIX socket: %d", errno);
        return -errno;
    }

    /* Remove any stale socket path from previous runs */
    unlink(DBUS_BROKER_SOCK_PATH);

    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
    };
    strncpy(addr.sun_path, DBUS_BROKER_SOCK_PATH,
            sizeof(addr.sun_path) - 1);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERR("Failed to bind AF_UNIX socket '%s': %d",
                DBUS_BROKER_SOCK_PATH, errno);
        close(fd);
        return -errno;
    }

    if (listen(fd, 128) < 0) {
        LOG_ERR("Failed to listen on AF_UNIX socket: %d", errno);
        close(fd);
        return -errno;
    }

    /* Set non-blocking so the broker can accept all pending connections
     * in a single dispatch cycle without blocking. */
    flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    LOG_INF("AF_UNIX listener created: fd=%d path=%s", fd,
            DBUS_BROKER_SOCK_PATH);
    return fd;
}

static int add_listener_to_broker(int listener_fd)
{
    int r;
    ControllerListener *listener = NULL;

    r = controller_add_listener(&g_broker->controller, &listener,
                                "/org/bus1/DBus/Listener/0",
                                listener_fd, NULL);
    if (r < 0) {
        LOG_ERR("Failed to add listener to broker: %s (code: %d)",
                strerror(-r), r);
        return r;
    }

    LOG_INF("AF_UNIX listener added to broker: fd=%d", listener_fd);
    return 0;
}

/* ================================================================== */
/* Broker deployment                                                   */
/* ================================================================== */
static int standard_broker_deployment(void)
{
    int r;
    int listener_fd = -1;

    /* Step 1: Create controller socketpair */
    const char *machine_id = "0123456789abcdef0123456789abcdef";

    r = socketpair(AF_UNIX, SOCK_STREAM, 0, g_controller_fds);
    if (r < 0) {
        LOG_ERR("socketpair failed: %d", r);
        return -errno;
    }
    LOG_INF("Socketpair created: [%d, %d]",
            g_controller_fds[0], g_controller_fds[1]);

    /* Step 2: Create broker */
    r = broker_new(&g_broker, NULL, machine_id, g_controller_fds[0],
                   1024 * 1024, 256, 64, 128);
    if (r < 0) {
        LOG_ERR("broker_new failed: %d", r);
        close(g_controller_fds[0]);
        close(g_controller_fds[1]);
        return r;
    }

    k_msleep(100);

    /* Step 3: Create AF_UNIX listener */
    listener_fd = create_listener_socket();
    if (listener_fd < 0) {
        LOG_ERR("Failed to create AF_UNIX listener: %d", listener_fd);
        broker_free(g_broker);
        g_broker = NULL;
        close(g_controller_fds[0]);
        close(g_controller_fds[1]);
        return listener_fd;
    }

    /* Step 4: Add listener to broker */
    r = add_listener_to_broker(listener_fd);
    if (r < 0) {
        LOG_ERR("Failed to add listener to broker: %d", r);
        close(listener_fd);
        broker_free(g_broker);
        g_broker = NULL;
        close(g_controller_fds[0]);
        close(g_controller_fds[1]);
        return r;
    }

    /* Update deployment state */
    k_mutex_lock(&deploy_state.lock, K_FOREVER);
    deploy_state.broker_ready = true;
    deploy_state.listener_fd = listener_fd;
    k_mutex_unlock(&deploy_state.lock);

    LOG_INF("Broker: %p, Listener FD: %d", g_broker, listener_fd);

    /* Give broker time to start event loop */
    k_msleep(1000);

    return 0;
}

static int deploy_standard_broker(void)
{
    int r;

    r = standard_broker_deployment();
    if (r < 0) {
        LOG_ERR("Standard deployment failed: %d", r);
        return r;
    }

    LOG_INF("Starting broker event loop (AF_UNIX: %s)...",
            DBUS_BROKER_SOCK_PATH);
    k_msleep(500);
    r = broker_run(g_broker);

    if (g_broker) {
        broker_free(g_broker);
        g_broker = NULL;
    }

    if (g_controller_fds[0] >= 0) {
        close(g_controller_fds[0]);
        close(g_controller_fds[1]);
        g_controller_fds[0] = g_controller_fds[1] = -1;
    }

    return r;
}

/*
 * Broker Thread
 */
static void broker_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    int r;

    r = deploy_standard_broker();

    if (r == 0) {
        LOG_INF("[DBus Broker] Broker completed successfully");
    } else {
        LOG_ERR("[DBus Broker] Broker exited with error: %d", r);
    }
}

static pthread_t broker_thread;

static void *broker_handler(void *arg)
{
    (void)arg;
    broker_thread_entry(NULL, NULL, NULL);
    return NULL;
}

/*
 * D-Bus Broker initialization
 */
static int dbus_broker_main(void)
{
    CREATE_TASK_WITH_PTHREAD(&broker_thread, broker,
                             DBUS_BROKER_THREAD_STACK_SIZE);

    LOG_INF("[DBus Broker] D-Bus Broker subsystem initialized");
    return 0;
}

SYS_INIT(dbus_broker_main, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

/* ================================================================== */
/* Public API                                                          */
/* ================================================================== */

int connect_to_dbroker(sd_bus **bus)
{
    int client_fd = -1;
    int r;
    int retry_count = 0;
    static bool broker_started = false;
    sd_bus *internal_bus = NULL;

    if (!bus) {
        LOG_ERR("[DBroker API] Invalid parameter: bus is NULL");
        return -EINVAL;
    }

    if (!broker_started) {
        k_msleep(1000);
        broker_started = true;
    }

    /* Retry connecting to the AF_UNIX listener until broker is ready.
     * Both socket() and connect() can fail transiently (broker listener
     * not yet created, AF_UNIX socket table full from stale connections,
     * etc.). We retry both. */
    while (retry_count < 200) {
        if (client_fd < 0) {
            client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (client_fd < 0) {
                /* socket() failed - retry after delay */
                if (retry_count < 5 || retry_count % 50 == 0) {
                    LOG_INF("[DBroker API] socket() failed on attempt "
                            "%d: %d (%s)", retry_count + 1, errno,
                            strerror(errno));
                }
                goto retry;
            }
        }

        struct sockaddr_un addr = {
            .sun_family = AF_UNIX,
        };
        strncpy(addr.sun_path, DBUS_BROKER_SOCK_PATH,
                sizeof(addr.sun_path) - 1);

        r = connect(client_fd, (struct sockaddr *)&addr, sizeof(addr));
        if (r >= 0) {
            break; /* Connected successfully */
        }

        /* connect() failed - close socket and retry */
        close(client_fd);
        client_fd = -1;
        if (retry_count < 5 || retry_count % 50 == 0) {
            LOG_INF("[DBroker API] Connection attempt %d failed, "
                    "errno: %d (%s)", retry_count + 1, errno,
                    strerror(errno));
        }

retry:
        retry_count++;
        k_msleep(100);
    }

    if (client_fd < 0) {
        LOG_ERR("[DBroker API] Failed to connect after %d retries",
                retry_count);
        return -ENOTCONN;
    }

    LOG_INF("[DBroker API] Connected to broker via AF_UNIX: fd=%d",
            client_fd);

    /* Set non-blocking for sd-bus */
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    }

    /* Create sd-bus using the connected socket */
    r = sd_bus_new(&internal_bus);
    if (r < 0) {
        LOG_ERR("[DBroker API] sd_bus_new failed: %d", r);
        close(client_fd);
        return r;
    }

    r = sd_bus_set_fd(internal_bus, client_fd, client_fd);
    if (r < 0) {
        LOG_ERR("[DBroker API] sd_bus_set_fd failed: %d", r);
        sd_bus_unref(internal_bus);
        close(client_fd);
        return r;
    }

    r = sd_bus_set_bus_client(internal_bus, true);
    if (r < 0) {
        LOG_ERR("[DBroker API] sd_bus_set_bus_client failed: %d", r);
        sd_bus_unref(internal_bus);
        close(client_fd);
        return r;
    }

    /* Allow sd_bus_default_system() to find this bus */
    r = sd_bus_set_default_system(internal_bus);
    if (r < 0 && r != -EEXIST) {
        LOG_WRN("[DBroker API] sd_bus_set_default_system: %d", r);
    }

    /* Start the bus (sends Hello message) */
    r = sd_bus_start(internal_bus);
    if (r < 0) {
        LOG_WRN("[DBroker API] sd_bus_start returned: %d", r);
    }

    /* Wait for bus to become ready (process Hello response) */
    retry_count = 0;
    while (!sd_bus_is_ready(internal_bus) && retry_count < 200) {
        r = sd_bus_process(internal_bus, NULL);
        if (r < 0) {
            LOG_ERR("[DBroker API] Failed to process bus messages: %d",
                    r);
            break;
        }
        k_msleep(25);
        retry_count++;
    }

    if (!sd_bus_is_ready(internal_bus)) {
        LOG_ERR("[DBroker API] Bus not ready after %d attempts",
                retry_count);
        sd_bus_unref(internal_bus);
        return -ETIMEDOUT;
    }

    LOG_DBG("[DBroker API] Bus ready after %d attempts", retry_count);

    *bus = internal_bus;
    printk("connected to broker\n");

    return 0;
}

int disconnect_from_dbroker(sd_bus *bus)
{
    if (!bus) {
        return -EINVAL;
    }

    LOG_INF("[sd-bus] Disconnecting from dbus-broker...");

    /*
     * sd_bus_flush_close_unref does all the work:
     *   sd_bus_flush  - flush pending outgoing messages
     *   sd_bus_close  - closes the AF_UNIX socket fd (via bus_close_io_fds)
     *   sd_bus_unref  - frees the bus object
     *
     * The broker's poll loop detects EOF on the connection fd (via
     * EPOLLHUP/POLLHUP), which triggers peer_dispatch → peer_free.
     * No explicit socketpool cleanup is needed with AF_UNIX sockets.
     *
     * We wait briefly after close to let the broker process the EOF
     * before the caller continues (avoids race on peer freeing).
     */
    sd_bus_flush(bus);
    sd_bus_close(bus);
    k_msleep(50);
    sd_bus_unref(bus);

    LOG_INF("[sd-bus] Disconnected successfully");
    return 0;
}
