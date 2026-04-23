/* SPDX-License-Identifier: Apache-2.0 */

/**
 * @file test_sd_event.c
 * @brief SD-Event event loop test suite - integrated into test_broker
 * 
 * This file runs all sd-event tests in a separate thread to provide
 * stress testing for the shared dispatch_context with dbus-broker.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_sd_event, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/time.h>
#include <sys/socket.h>

/* Include sd-event test suite from test_event sample */
// #include "../../test_event/sd_event_test.h"

/* Test thread stack size - increased for comprehensive tests */
#define SD_EVENT_TEST_STACK_SIZE 16384UL

/* Test thread priority - lower than main test thread to avoid interference */
#define SD_EVENT_TEST_PRIORITY 5

/* Forward declaration of the main test function from sd_event_test.c */
extern int run_sd_event_all_tests(void);

/**
 * @brief SD-Event test thread entry point
 * 
 * This thread runs the complete sd-event test suite concurrently
 * with dbus-broker tests to stress-test the shared dispatch_context.
 */
static void sd_event_test_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    int failed;
    
    LOG_INF("========================================");
    LOG_INF("SD-Event Test Suite Starting");
    LOG_INF("Running concurrently with dbus-broker tests");
    LOG_INF("This provides stress testing for dispatch_context");
    LOG_INF("========================================");

    // k_sleep(K_SECONDS(3));
    
    /* Run all sd-event tests */
    for (int i = 0; i < 10; i++)
    {
        failed = run_sd_event_all_tests();
        
        if (failed == 0) {
            LOG_INF("✓ All SD-Event tests PASSED - Round %d!", i+1);
        } else {
            LOG_ERR("✗ %d SD-Event test(s) FAILED - Round %d!", failed, i+1);
        }
    }
    
    LOG_INF("SD-Event test thread exiting");
}

/* Define the test thread */
// K_THREAD_DEFINE(sd_event_test_thread,
//                 SD_EVENT_TEST_STACK_SIZE,
//                 sd_event_test_thread_entry,
//                 NULL, NULL, NULL,
//                 SD_EVENT_TEST_PRIORITY,
//                 0,
//                 0);
