#include "thread_dependency_mgr.hpp"

#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(TEST_BROKER, LOG_LEVEL_DBG);

// Thread registry
#define MAX_THREADS 64
static thread_descriptor_t *registered_threads[MAX_THREADS];
static int registered_thread_count = 0;
static struct k_mutex thread_registry_mutex;
static bool registry_initialized = false;

// Initialize registry
static void init_registry(void)
{
    if (!registry_initialized) {
        k_mutex_init(&thread_registry_mutex);
        registry_initialized = true;
    }
}

// Data structures for circular dependency detection
#define MAX_CALL_STACK_DEPTH 32
static const char *call_stack[MAX_CALL_STACK_DEPTH];
static int call_stack_depth = 0;
static struct k_mutex call_stack_mutex;
static bool call_stack_initialized = false;

// Initialize call stack mutex
static void init_call_stack(void)
{
    if (!call_stack_initialized) {
        k_mutex_init(&call_stack_mutex);
        call_stack_initialized = true;
    }
}

// Check for circular dependencies
static bool is_circular_dependency(const char *thread_name)
{
    for (int i = 0; i < call_stack_depth; i++) {
        if (strcmp(call_stack[i], thread_name) == 0) {
            return true;
        }
    }
    return false;
}

// Add to call stack
static bool push_call_stack(const char *thread_name)
{
    if (call_stack_depth >= MAX_CALL_STACK_DEPTH) {
        LOG_ERR("Error: Call stack overflow (max depth: %d)", MAX_CALL_STACK_DEPTH);
        return false;
    }
    call_stack[call_stack_depth++] = thread_name;
    return true;
}

// Remove from call stack
static void pop_call_stack(void)
{
    if (call_stack_depth > 0) {
        call_stack_depth--;
    }
}

void register_thread_descriptor(thread_descriptor_t *desc, const char **deps, int dep_count)
{
    init_registry();

    k_mutex_lock(&thread_registry_mutex, K_FOREVER);

    if (registered_thread_count < MAX_THREADS) {
        // Initialize dependency count
        desc->dependency_count = 0;

        // Special handling: validate dependency array
        if (dep_count > 0 && deps != nullptr) {
            int valid_deps = 0;
            for (int i = 0; i < dep_count && i < MAX_DEPENDENCIES; i++) {
                // Check if dependency is valid (not null and not empty string)
                if (deps[i] != nullptr && strlen(deps[i]) > 0) {
                    desc->dependencies[valid_deps] = deps[i];
                    valid_deps++;
                } else {
                    // Stop processing on invalid dependency
                    LOG_WRN("Warning: Invalid dependency at index %d for "
                        "thread %s",
                        i, desc->name);
                    break;
                }
            }
            desc->dependency_count = valid_deps;

            // Verify calculated dependency count matches actual valid dependencies
            if (dep_count != valid_deps) {
                LOG_WRN("Warning: Calculated dep_count (%d) differs from valid "
                    "deps (%d) for thread %s",
                    dep_count, valid_deps, desc->name);
            }
        }

        registered_threads[registered_thread_count++] = desc;
        LOG_INF("Registered thread: %s with %d dependencies", desc->name,
            desc->dependency_count);

        // Print dependencies
        for (int i = 0; i < desc->dependency_count; i++) {
            LOG_INF("  - depends on: %s", desc->dependencies[i]);
        }
    } else {
        LOG_ERR("Error: Too many threads registered (max: %d)", MAX_THREADS);
    }

    k_mutex_unlock(&thread_registry_mutex);
}

// Find thread descriptor
thread_descriptor_t *find_thread_descriptor(const char *name)
{
    init_registry();

    k_mutex_lock(&thread_registry_mutex, K_FOREVER);

    thread_descriptor_t *result = nullptr;
    for (int i = 0; i < registered_thread_count; i++) {
        if (strcmp(registered_threads[i]->name, name) == 0) {
            result = registered_threads[i];
            break;
        }
    }

    k_mutex_unlock(&thread_registry_mutex);
    return result;
}

// Check if thread dependencies are satisfied
bool check_thread_dependencies(const char *thread_name)
{
    thread_descriptor_t *desc = find_thread_descriptor(thread_name);
    if (!desc) {
        LOG_ERR("Error: Thread '%s' not found", thread_name);
        return false;
    }

    // Check all dependencies
    for (int i = 0; i < desc->dependency_count; i++) {
        thread_descriptor_t *dep_desc = find_thread_descriptor(desc->dependencies[i]);
        if (!dep_desc) {
            LOG_ERR("Error: Dependency '%s' for thread '%s' not found",
                  desc->dependencies[i], thread_name);
            return false;
        }

        if (dep_desc->state != THREAD_STATE_READY) {
            LOG_INF("Dependency '%s' for thread '%s' not ready (state: %d)",
                desc->dependencies[i], thread_name, dep_desc->state);
            return false;
        }
    }

    return true;
}

// Wait for dependency thread to become ready
static int wait_for_dependency_ready(thread_descriptor_t *dep_desc, const char *dep_name,
                     int timeout_s)
{
    if (dep_desc->state == THREAD_STATE_READY) {
        return 0;
    }

    if (dep_desc->state == THREAD_STATE_RUNNING) {
        if (dep_desc->ready_sem) {
            LOG_INF("Waiting for thread '%s' to be ready (timeout: %ds)...", dep_name,
                timeout_s);
            int ret = k_sem_take(dep_desc->ready_sem, K_SECONDS(timeout_s));
            if (ret == 0) {
                LOG_INF("Thread '%s' is ready", dep_name);
                return 0;
            } else {
                LOG_ERR("Error: Timeout waiting for thread '%s' to be ready",
                      dep_name);
                return -1;
            }
        } else {
            LOG_INF("Warning: Thread '%s' has no ready semaphore, assuming ready",
                dep_name);
            return 0;
        }
    } else if (dep_desc->state == THREAD_STATE_FAILED) {
        LOG_ERR("Error: Dependency thread '%s' is in FAILED state", dep_name);
        return -1;
    } else {
        LOG_ERR("Error: Dependency thread '%s' is in invalid state %d", dep_name,
              dep_desc->state);
        return -1;
    }
}

// Start a single thread and its dependencies
int start_thread_with_dependencies(const char *thread_name)
{
    if (!thread_name) {
        LOG_ERR("Error: thread_name is NULL");
        return -1;
    }

    init_call_stack();

    // Acquire call stack lock to prevent concurrent access
    k_mutex_lock(&call_stack_mutex, K_FOREVER);

    // Check for circular dependencies
    if (is_circular_dependency(thread_name)) {
        LOG_ERR("Error: Circular dependency detected for thread '%s'", thread_name);
        LOG_ERR("Call stack: ");
        for (int i = 0; i < call_stack_depth; i++) {
            LOG_ERR("%s -> ", call_stack[i]);
        }
        LOG_ERR("%s", thread_name);
        k_mutex_unlock(&call_stack_mutex);
        return -1;
    }

    // Add to call stack
    if (!push_call_stack(thread_name)) {
        k_mutex_unlock(&call_stack_mutex);
        return -1;
    }

    k_mutex_unlock(&call_stack_mutex);

    // Find thread descriptor
    thread_descriptor_t *desc = find_thread_descriptor(thread_name);
    if (!desc) {
        LOG_ERR("Error: Thread '%s' not found", thread_name);
        k_mutex_lock(&call_stack_mutex, K_FOREVER);
        pop_call_stack();
        k_mutex_unlock(&call_stack_mutex);
        return -1;
    }

    LOG_INF("Processing thread '%s' (deps: %d, state: %d)", desc->name, desc->dependency_count,
        desc->state);

    // If already started or starting, return immediately
    if (desc->state == THREAD_STATE_READY || desc->state == THREAD_STATE_RUNNING) {
        k_mutex_lock(&call_stack_mutex, K_FOREVER);
        pop_call_stack();
        k_mutex_unlock(&call_stack_mutex);
        return 0;
    }

    // Do not restart if failed
    if (desc->state == THREAD_STATE_FAILED) {
        LOG_ERR("Error: Thread '%s' is in FAILED state", thread_name);
        k_mutex_lock(&call_stack_mutex, K_FOREVER);
        pop_call_stack();
        k_mutex_unlock(&call_stack_mutex);
        return -1;
    }

    int ret = 0;

    // Recursively start dependencies
    for (int i = 0; i < desc->dependency_count; i++) {
        const char *dep_name = desc->dependencies[i];

        LOG_INF("Starting dependency '%s' for thread '%s'", dep_name, thread_name);

        ret = start_thread_with_dependencies(dep_name);
        if (ret != 0) {
            LOG_ERR("Error: Failed to start dependency '%s' for thread '%s'",
                  dep_name, thread_name);
            goto cleanup;
        }

        // Wait for dependency to actually become ready
        thread_descriptor_t *dep_desc = find_thread_descriptor(dep_name);
        if (!dep_desc) {
            LOG_ERR("Error: Dependency thread '%s' not found", dep_name);
            ret = -1;
            goto cleanup;
        }

        // Wait for dependency to be ready (100 seconds timeout)
        ret = wait_for_dependency_ready(dep_desc, dep_name, 20);
        if (ret != 0) {
            LOG_ERR("Error: Dependency '%s' failed to become ready", dep_name);
            goto cleanup;
        }
    }

    // Start current thread
    LOG_INF("Starting thread '%s'...", thread_name);

    ret = desc->init_func();
    if (ret == 0) {
        desc->state = THREAD_STATE_RUNNING;
        // Wait for thread to signal readiness
        if (desc->ready_sem) {
            int sem_ret =
                k_sem_take(desc->ready_sem, K_SECONDS(100)); // 100 seconds timeout
            if (sem_ret == 0) {
                desc->state = THREAD_STATE_READY;
                LOG_INF("Thread '%s' started successfully", thread_name);
            } else {
                LOG_ERR("Error: Timeout waiting for thread '%s' ready signal",
                      thread_name);
                desc->state = THREAD_STATE_FAILED;
                ret = -1;
            }
        } else {
            // Without semaphore, set ready directly
            desc->state = THREAD_STATE_READY;
            LOG_INF("Thread '%s' started successfully (no semaphore)", thread_name);
        }
    } else {
        desc->state = THREAD_STATE_FAILED;
        LOG_ERR("Error: Thread '%s' failed to start (ret: %d)", thread_name, ret);
    }

cleanup:
    // Clean up call stack
    k_mutex_lock(&call_stack_mutex, K_FOREVER);
    pop_call_stack();
    k_mutex_unlock(&call_stack_mutex);

    return ret;
}

// Start all auto-start threads
int start_all_auto_threads(void)
{
    init_registry();

    LOG_INF("Starting all auto-start threads...");

    int failed_count = 0;
    for (int i = 0; i < registered_thread_count; i++) {
        thread_descriptor_t *desc = registered_threads[i];
        if (desc->auto_start && desc->state == THREAD_STATE_INITIAL) {
            int ret = start_thread_with_dependencies(desc->name);
            if (ret != 0) {
                failed_count++;
                LOG_ERR("Failed to start thread '%s'", desc->name);
            }
        }
    }

    if (failed_count == 0) {
        LOG_INF("All auto-start threads started successfully");
    } else {
        LOG_WRN("Warning: %d threads failed to start", failed_count);
    }

    return failed_count;
}

// Print thread status
void print_thread_status(void)
{
    init_registry();

    LOG_INF("\n=== Thread Status ===");
    for (int i = 0; i < registered_thread_count; i++) {
        thread_descriptor_t *desc = registered_threads[i];
        const char *state_str;

        switch (desc->state) {
        case THREAD_STATE_INITIAL:
            state_str = "INITIAL";
            break;
        case THREAD_STATE_RUNNING:
            state_str = "STARTING";
            break;
        case THREAD_STATE_READY:
            state_str = "READY";
            break;
        case THREAD_STATE_FAILED:
            state_str = "FAILED";
            break;
        default:
            state_str = "UNKNOWN";
            break;
        }

        LOG_INF("Thread: %-20s State: %-8s Auto: %s", desc->name, state_str,
            desc->auto_start ? "YES" : "NO");

        // Print dependency count and details
        LOG_INF("  Dependencies Count: %d", desc->dependency_count);
    }
    LOG_INF("=====================\n");
}
