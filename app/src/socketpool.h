
/* SPDX-License-Identifier: Apache-2.0 */
#ifndef __SOCKETPOOL_H__
#define __SOCKETPOOL_H__

#include <zephyr/kernel.h>

/* Forward declaration */
struct Broker;
typedef struct Broker Broker;

/**
 * @brief Initialize the socketpool
 * 
 * @return 0 on success, negative errno on failure
 */
int socketpool_init(void);

/**
 * @brief Allocate a socketpair from the pool
 * 
 * @param broker_fd Pointer to store the broker's fd
 * @param client_fd Pointer to store the client's fd
 * @return 0 on success, negative errno on failure
 */
int socketpool_allocate(int *broker_fd, int *client_fd);

/**
 * @brief Free a socketpair back to the pool
 * 
 * @param broker_fd The broker's fd
 * @param client_fd The client's fd
 * @return 0 on success, negative errno on failure
 */
int socketpool_free(int broker_fd, int client_fd);

/**
 * @brief Add a peer to the broker using a socketpair from the pool
 * @details This function is called when a client requests a connection
 * @param broker Pointer to the broker instance
 * @param broker_fd The broker's fd from the socketpair
 * @return 0 on success, negative error code on failure
 */
int socketpool_add_peer_to_broker(Broker *broker, int broker_fd);

/**
 * @brief Check if a fd is managed by the socketpool
 * @param fd The file descriptor to check
 * @return true if the fd is managed by the pool, false otherwise
 */
bool socketpool_is_pool_fd(int fd);

/**
 * @brief Wake up the broker's poll loop
 * @param broker The broker instance
 * 
 * Write to the broker's terminate pipe to wake up poll when new data
 * has been written to a peer fd. Zephyr's poll may not wake
 * up for socketpair data arriving while poll is already blocking.
 */
void socketpool_wake_broker(Broker *broker);

#endif /* __SOCKETPOOL_H__ */
