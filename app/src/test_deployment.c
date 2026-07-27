/*
 * Corrected D-Bus Broker Deployment Model
 * Using AF_INET sockets for Zephyr compatibility
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

/* INADDR_LOOPBACK is not defined in Zephyr */
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK 0x7f000001
#endif

/* strerror is not available in Zephyr */
#ifndef strerror
char *strerror(int errnum);
#endif

#include "dbus_broker.h"
#include "broker/broker.h"
#include <systemd/sd-bus.h>
#include <systemd/sd-bus-vtable.h>

LOG_MODULE_REGISTER(BROKER_DEPLOY, LOG_LEVEL_DBG);

/* External variables */
extern int g_controller_fds[2];
extern Broker *g_broker;
extern struct deployment_state deploy_state;

/*
 * Persistent log context for the broker. dbus-broker stores a pointer to this
 * (broker->log / bus->log) and dereferences it from many code paths (e.g.
 * log_append_common, bus_log_append_sender, log_commitf). Passing NULL here
 * (as was done before) makes every one of those paths fault with a NULL-pointer
 * load the moment the broker tries to log anything (e.g. a peer sending a
 * malformed message). A log_init()'d context in LOG_MODE_NONE is safe: the
 * Zephyr port routes it through printk instead of touching the journal buffer.
 * Must be file-scope/static so the pointer stays valid for the broker's lifetime.
 */
static Log g_broker_log;

/* Controller bus for sending D-Bus method calls to broker */
static sd_bus *g_controller_bus = NULL;

/* Service context */
struct deployment_service {
    int total_calls;
    int active_connections;
    struct k_mutex stats_mutex;
};

static struct deployment_service g_service = {
    .total_calls = 0,
    .active_connections = 0
};

struct deployment_state {
    struct k_mutex lock;
    bool broker_ready;
    int listener_fd;
    bool service_provider_ready;
};

/* Service methods */
static int method_get_statistics(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    ARG_UNUSED(ret_error);
    struct deployment_service *service = userdata;

    k_mutex_lock(&service->stats_mutex, K_FOREVER);
    int calls = service->total_calls;
    int connections = service->active_connections;
    k_mutex_unlock(&service->stats_mutex);

    LOG_INF("Statistics requested: %d calls, %d connections", calls, connections);
    return sd_bus_reply_method_return(m, "(ii)", calls, connections);
}

static int method_process_data(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    ARG_UNUSED(ret_error);
    struct deployment_service *service = userdata;
    const char *data;
    int r;

    r = sd_bus_message_read(m, "s", &data);
    if (r < 0) return r;

    k_mutex_lock(&service->stats_mutex, K_FOREVER);
    service->total_calls++;
    int call_num = service->total_calls;
    k_mutex_unlock(&service->stats_mutex);

    LOG_INF("Processing data #%d: '%s'", call_num, data);
    return sd_bus_reply_method_return(m, "s", "PROCESSED");
}

/* VTable */
static const sd_bus_vtable service_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("GetStatistics", NULL, "(ii)", method_get_statistics, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("ProcessData", "s", "s", method_process_data, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};

#define NUM_CLIENTS 13
K_THREAD_STACK_ARRAY_DEFINE(client_stacks, NUM_CLIENTS, 4096);
static struct k_thread client_threads[NUM_CLIENTS];

/* Synchronization for concurrent connection testing */
static K_SEM_DEFINE(client_start_sem, 0, NUM_CLIENTS);

#define SERVICE_NAME "com.deployment.Service"
#define SERVICE_PATH "/com/deployment/Service"

/* Service provider thread - registers and runs the service */
static void service_provider_task(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    sd_bus *service_bus = NULL;
    int r;
    int retry_count = 0;
    const int max_retries = 50; /* Wait up to 5 seconds */
    // int client_id = (int)(uintptr_t)p1;

    while ((g_broker == NULL || g_controller_fds[0] < 0 || !deploy_state.broker_ready)
            && retry_count < max_retries) {
        LOG_DBG("[Service Provider] Waiting... retry %d, g_broker: %p, fd: %d",
                retry_count, g_broker, g_controller_fds[0]);
        k_msleep(100);
        retry_count++;
    }

    k_msleep(500);

    LOG_INF("[Service Provider] Starting...");

    r = connect_to_dbroker(&service_bus);
    if (r < 0) {
        LOG_ERR("[Service Provider] Connection to dbus-broker failed, error: %d", r);
        goto cleanup;
    }

    LOG_INF("[Service Provider] Connection established, service_bus: %p", service_bus);
    LOG_INF("[Service Provider] Calling sd_bus_request_name for '%s'", SERVICE_NAME);
    /* Request service name */
    r = sd_bus_request_name(service_bus, SERVICE_NAME, 0);
    if (r < 0) {
        LOG_WRN("[Service Provider] Failed to request name '%s', error: %d (errno: %d, strerror: %s)", 
                SERVICE_NAME, r, errno, strerror(errno));
        /* Continue anyway for testing */
    } else {
        LOG_INF("[Service Provider] Successfully requested name '%s', return value: %d", SERVICE_NAME, r);
    }

    /* Register service object */
    r = sd_bus_add_object_vtable(service_bus, NULL, SERVICE_PATH, SERVICE_NAME, service_vtable, &g_service);
    if (r < 0) {
        LOG_ERR("[Service Provider] Failed to register vtable, error: %d", r);
        return;
    }

    // LOG_INF("[Service Provider] Service registered successfully");

    /* Signal that service provider is ready */
    k_mutex_lock(&deploy_state.lock, K_FOREVER);
    deploy_state.service_provider_ready = true;
    k_mutex_unlock(&deploy_state.lock);
    LOG_INF("[Service Provider] Service provider is now ready");

    /* Process events */
    while (true) {
        r = sd_bus_process(service_bus, NULL);
        if (r < 0) {
            LOG_ERR("[Service Provider] Process failed, error: %d", r);
            break;
        }
        if (r == 0) {
            r = sd_bus_wait(service_bus, 50000); /* 1 second timeout */
            if (r < 0) {
                LOG_ERR("[Service Provider] Wait failed, error: %d", r);
                break;
            }
        }
    }

cleanup:
    disconnect_from_dbroker(service_bus);
    LOG_INF("[Service Provider] Exiting");
}

/* Service provider thread definition */
// K_THREAD_DEFINE(service_provider_thread, 4096, service_provider_task, NULL, NULL, NULL, 6, 0, 0);

/* Client connection handler */
static void client_connection_task(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    int client_id = (int)(uintptr_t)p1;
    sd_bus *client_bus = NULL;
    int r;

    LOG_INF("Client %d: Starting connection", client_id);

    /*
     * Wait for all client threads to be ready before starting connections.
     * This ensures true concurrent connection testing.
     */
    k_sem_take(&client_start_sem, K_FOREVER);

    r = connect_to_dbroker(&client_bus);
    if (r < 0) {
        LOG_ERR("Client %d: Connection to dbus-broker failed, error: %d", client_id, r);
        goto cleanup;
    }

    /* Update connection count */
    k_mutex_lock(&g_service.stats_mutex, K_FOREVER);
    g_service.active_connections++;
    int conn_count = g_service.active_connections;
    k_mutex_unlock(&g_service.stats_mutex);

    LOG_INF("Client %d: Connected (total connections: %d)", client_id, conn_count);

    // k_msleep(200);

    /* Test 2: ListNames method */
    LOG_INF("Testing ListNames method...");
    sd_bus_message *reply = NULL;
    r = sd_bus_call_method(client_bus,
                          "org.freedesktop.DBus",
                          "/org/freedesktop/DBus",
                          "org.freedesktop.DBus",
                          "ListNames",
                          NULL,
                          &reply,
                          NULL);
    if (r < 0) {
        LOG_ERR("ListNames failed: error code %d", r);
    } else {
        LOG_INF("✓ ListNames succeeded (reply: %p)", (void *)reply);
        
        /* 解析并打印返回的names列表 */
        int r2 = sd_bus_message_enter_container(reply, 'a', "s");
        if (r2 < 0) {
            LOG_ERR("Failed to enter array container: error code %d", r2);
        } else {
            LOG_INF("D-Bus names on the bus:");
            int count = 0;
            while (1) {
                const char *name = NULL;
                r2 = sd_bus_message_read(reply, "s", &name);
                if (r2 <= 0) {
                    if (r2 < 0) {
                        LOG_ERR("Error reading name: error code %d", r2);
                    }
                    break;
                }
                LOG_INF("  [%d] %s", ++count, name);
            }
            
            if (count == 0) {
                LOG_INF("  (No names found)");
            } else {
                LOG_INF("Total names: %d", count);
            }
        }
        
        /* 离开数组容器 */
        sd_bus_message_exit_container(reply);
        sd_bus_message_unref(reply);
    }
    
    k_msleep(200);

    /* Test 3: Your custom service methods */
    LOG_INF("Testing custom service methods...");
    
    /* GetStatistics */
    sd_bus_message *stats_reply = NULL;
    r = sd_bus_call_method(client_bus,
                          "com.deployment.Service",
                          "/com/deployment/Service",
                          "com.deployment.Service",
                          "GetStatistics",
                          NULL,
                          &stats_reply,
                          NULL);
    if (r < 0) {
        LOG_ERR("GetStatistics failed: %d", r);
    } else {
        int calls, connections;
        r = sd_bus_message_read(stats_reply, "(ii)", &calls, &connections);
        if (r >= 0) {
            LOG_INF("✓ GetStatistics: %d calls, %d connections", calls, connections);
        }
        sd_bus_message_unref(stats_reply);
    }
    
    k_msleep(200);

    /* ProcessData */
    sd_bus_message *process_reply = NULL;
    r = sd_bus_call_method(client_bus,
                          "com.deployment.Service",
                          "/com/deployment/Service", 
                          "com.deployment.Service",
                          "ProcessData",
                          NULL,
                          &process_reply,
                          "s",
                          "test data from client");
    if (r < 0) {
        LOG_ERR("ProcessData failed: %d", r);
    } else {
        const char *result;
        r = sd_bus_message_read(process_reply, "s", &result);
        if (r >= 0) {
            LOG_INF("✓ ProcessData result: %s", result);
        }
        sd_bus_message_unref(process_reply);
    }

    // LOG_INF("Heap remaining: %d bytes", k_mem_free_get());
    // LOG_INF("Active connections: %d", g_service.active_connections);

    k_mutex_lock(&g_service.stats_mutex, K_FOREVER);
    g_service.active_connections--;
    k_mutex_unlock(&g_service.stats_mutex);

cleanup:
    /* necessary to ensure the broker has processed the request *
     * recommendation: 50ms for unix socket, 200ms for net socket
     * for 20 clients. if less clients, can decrease the delay */
    if (client_bus) {
        disconnect_from_dbroker(client_bus);
    }

    LOG_INF("=== CLIENT COMMUNICATION TEST COMPLETE ===");
    // LOG_INF("Heap remaining: %d bytes", k_mem_free_get());
    // LOG_INF("Active connections: %d", g_service.active_connections);
}

#define LISTENER_SOCKET_PATH "/tmp/dbus-test"

/* Add listener to broker via direct controller API */
int add_listener_to_broker(int listener_fd)
{
    int r;
    ControllerListener *listener = NULL;

    /* Use controller's direct API to add listener */
    r = controller_add_listener(&g_broker->controller, &listener,
                                "/org/bus1/DBus/Listener/0",
                                listener_fd, NULL);
    if (r < 0) {
        LOG_ERR("Failed to append listener fd: %s (code: %d)", strerror(-r), r);
        return r;
    }

    LOG_INF("✓ Listener added to broker successfully");
    return 0;
}

static int setup_controller_bus(void)
{
    LOG_INF("=== CONTROLLER BUS SETUP (Skipped on Zephyr) ===");
    LOG_INF("On Zephyr, we use direct controller API calls instead of D-Bus bus");
    LOG_INF("Controller fd %d remains open for broker use", g_controller_fds[1]);
    return 0;
}

int create_listener_socket_with_retry(void)
{
    int fd = -1;
    int retries = 50;
    int delay_ms = 100;

    LOG_INF("=== CREATING LISTENER SOCKET WITH RETRY ===");

    while (retries > 0) {
        LOG_DBG("Attempt %d/%d to create listener socket...", 51-retries, 50);

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd >= 0) {
            LOG_DBG("✓ Socket created successfully on attempt %d", 51-retries);

             /* Set socket options */
            int reuse = 1;
            LOG_DBG("Setting SO_REUSEADDR...");
            if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
                LOG_WRN("Failed to set SO_REUSEADDR: %s (errno: %d)",
                        strerror(errno), errno);
            } else {
                LOG_DBG("✓ SO_REUSEADDR set successfully");
            }

            /*
             * CRITICAL FIX: Listener socket MUST be set to non-blocking mode for
             * concurrent connection handling. The reason is:
             *
             * 1. When multiple clients connect simultaneously, they all queue in the
             *    listen backlog
             * 2. The broker's listener_dispatch() loops accepting ALL pending connections
             * 3. If the listener is blocking, zsock_accept() will block after processing
             *    the first connection, preventing subsequent connections from being accepted
             * 4. Clients waiting in backlog will timeout (ETIMEDOUT) or return EALREADY
             *
             * With non-blocking mode:
             * - listener_dispatch() can accept all connections in a single dispatch cycle
             * - No blocking allows event loop to continue processing other events
             * - Connection queue is properly drained
             */
            int flags = fcntl(fd, F_GETFL, 0);
            if (flags < 0) {
                LOG_WRN("Failed to get socket flags: %s (errno: %d)",
                        strerror(errno), errno);
            } else if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
                LOG_WRN("Failed to set non-blocking mode: %s (errno: %d)",
                        strerror(errno), errno);
            } else {
                LOG_DBG("✓ Non-blocking mode set successfully");
            }

            /* Quick bind attempt */
            struct sockaddr_in addr = {0};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            addr.sin_port = htons(55555);

            if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                /* Use a larger backlog to handle concurrent connections */
                int backlog = 128;  /* Increase backlog for concurrent connections */
                if (listen(fd, backlog) == 0) {
                    LOG_INF("✓ Listener socket created successfully: fd=%d (backlog=%d)", fd, backlog);
                    return fd;
                } else {
                    LOG_DBG("Listen failed: %s (errno: %d)", strerror(errno), errno);
                }
            } else {
                LOG_DBG("Bind failed: %s (errno: %d)", strerror(errno), errno);
            }

            /* If bind/listen failed, close and retry */
            close(fd);
            fd = -1;
        } else {
            LOG_DBG("zsock_socket failed: %s (errno: %d)", strerror(errno), errno);
        }

        LOG_DBG("Retry %d failed, waiting %d ms...", 51-retries, delay_ms);
        k_msleep(delay_ms);
        delay_ms *= 2;  // Exponential backoff
        retries--;
    }

    LOG_ERR("Failed to create listener socket after all retries");
    return -EIO;
}

/* Modified deployment function */
static int standard_broker_deployment(void)
{
    int r;
    int listener_fd = -1;

    LOG_INF("===========================================");
    LOG_INF("STANDARD BROKER DEPLOYMENT (PERMISSIVE MODE)");
    LOG_INF("===========================================");

    /* Step 1: Create controller socketpair */
    const char *machine_id = "0123456789abcdef0123456789abcdef";
    r = socketpair(AF_UNIX, SOCK_STREAM, 0, g_controller_fds);
    if (r < 0) {
        LOG_ERR("socketpair failed: %s", strerror(errno));
        return -errno;
    }
    LOG_INF("✓ Socketpair created: [%d, %d]", g_controller_fds[0], g_controller_fds[1]);

    /* Step 2: Create broker FIRST (this might help with timing) */
    log_init(&g_broker_log);
    r = broker_new(&g_broker, &g_broker_log, machine_id, g_controller_fds[0],
                   1024*1024, 256, 64, 128);
    if (r < 0) {
        LOG_ERR("broker_new failed: %s", strerror(-r));
        close(g_controller_fds[0]);
        close(g_controller_fds[1]);
        return r;
    }
    LOG_INF("✓ Broker created: %p", g_broker);

    /* Small delay to let broker initialize */
    k_msleep(100);

    /* Step 3: Setup controller bus (permissive) */
    r = setup_controller_bus();
    if (r < 0) {
        LOG_ERR("Controller bus setup failed: %d", r);
        // Continue anyway in permissive mode
    }

    /* Step 4: Create listener socket with retry */
    listener_fd = create_listener_socket_with_retry();
    if (listener_fd < 0) {
        LOG_ERR("Failed to create listener socket: %d", listener_fd);
        // This is critical, so we do fail here
        return listener_fd;
    }

    /* Step 5: Add listener to broker */
    r = add_listener_to_broker(listener_fd);
    if (r < 0) {
        LOG_ERR("Failed to add listener to broker: %d", r);
        close(listener_fd);
        return r;
    }
    LOG_INF("✓ Listener fd=%d added to broker, 127.0.0.1:55555 should be listening now", listener_fd);

    /* Update deployment state */
    k_mutex_lock(&deploy_state.lock, K_FOREVER);
    deploy_state.broker_ready = true;
    deploy_state.listener_fd = listener_fd;
    k_mutex_unlock(&deploy_state.lock);

    LOG_INF("===========================================");
    LOG_INF("DEPLOYMENT COMPLETED SUCCESSFULLY!");
    LOG_INF("Broker: %p, Listener FD: %d", g_broker, listener_fd);
    LOG_INF("===========================================");

    k_msleep(1000);  /* Give broker time to start event loop and begin listening */

    return 0;
}

/* Main deployment function */
int deploy_standard_broker(void)
{
    int r;

    r = standard_broker_deployment();
    if (r < 0) {
        LOG_ERR("Standard deployment failed: %s", strerror(-r));
        return r;
    }

    /* Run broker (handles all client connections in event loop) */
    LOG_INF("Starting broker event loop (will now accept connections on 127.0.0.1:55555)...");
    LOG_INF("Service providers can now connect!");
    k_msleep(500);  /* Give service providers a moment to see this log */
    r = broker_run(g_broker);

    /* Cleanup */
    if (g_controller_bus) {
        sd_bus_unref(g_controller_bus);
        g_controller_bus = NULL;
    }

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

/* Test function */
int test_broker_deployment(void)
{
    if (!g_broker) return -ENODEV;

    LOG_INF("=== BROKER DEPLOYMENT RESULTS ===");
    LOG_INF("✓ Broker: %p", g_broker);
    LOG_INF("✓ Service: %s", SERVICE_NAME);
    LOG_INF("✓ Total calls processed: %d", g_service.total_calls);
    LOG_INF("✓ Peak connections: %d", g_service.active_connections);

    return 0;
}

/* Public entry point */
int run_broker_deployment(void)
{
    return deploy_standard_broker();
}

/* Function to start client threads */
int start_client_threads(void)
{
    LOG_INF("=== STARTING CLIENT THREADS ===");

    /* Create all client threads first without delay */
    for (int i = 0; i < NUM_CLIENTS; i++) {
        k_thread_create(&client_threads[i],
                       client_stacks[i],
                       K_THREAD_STACK_SIZEOF(client_stacks[i]),
                       client_connection_task,
                       (void *)(uintptr_t)(i + 1),
                       NULL, NULL,
                       5, 0, K_NO_WAIT);
        k_msleep(10 * (i+1));
        LOG_INF("Client thread %d started", i + 1);
    }

    /*
     * Give threads a moment to start and reach the synchronization point.
     * All threads are now waiting on client_start_sem.
     */
    k_msleep(50);

    /*
     * Release all threads simultaneously for true concurrent connection testing.
     * This ensures all clients call connect() at nearly the same time.
     */
    for (int i = 0; i < NUM_CLIENTS; i++) {
        k_sem_give(&client_start_sem);
    }

    LOG_INF("=== CLIENT THREADS STARTED ===");
    return 0;
}
