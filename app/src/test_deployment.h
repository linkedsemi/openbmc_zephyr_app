/*
 * test_deployment.h - Standard Broker Deployment Interface
 */

#ifndef TEST_DEPLOYMENT_H
#define TEST_DEPLOYMENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Main deployment function using broker_run() */
int run_broker_deployment(void);

/* Test function to verify deployment */
int test_broker_deployment(void);

int add_listener_to_broker(int listener_fd);
int create_listener_socket_with_retry(void);
int setup_controller_bus(void);

/* External variables for broker communication */
extern int g_controller_fds[2];
extern Broker *g_broker;

/* Function to start client threads */
int start_client_threads(void);

struct deployment_state {
    struct k_mutex lock;
    bool broker_ready;
    int listener_fd;
    bool service_provider_ready;
};

#ifdef __cplusplus
}
#endif

#endif /* TEST_DEPLOYMENT_H */
