#ifndef THREAD_DEPENDENCY_MGR_HPP
#define THREAD_DEPENDENCY_MGR_HPP

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#ifdef __cplusplus
extern "C" {
#endif


// Maximum number of dependencies
#define MAX_DEPENDENCIES    8
#define MAX_THREAD_NAME_LEN 32

// Thread state enumeration
typedef enum {
    THREAD_STATE_INITIAL = 0, // Not created
    THREAD_STATE_RUNNING,     // Executing entry function
    THREAD_STATE_READY,       // Business logic initialization complete
    THREAD_STATE_FAILED       // Initialization failed
} thread_state_t;

// Thread descriptor structure
typedef struct thread_descriptor {
    char name[MAX_THREAD_NAME_LEN];
    int (*init_func)(void);
    struct k_sem *ready_sem;
    thread_state_t state;
    const char *dependencies[MAX_DEPENDENCIES];
    int dependency_count;
    bool auto_start;
} thread_descriptor_t;

// Internal function declarations
void register_thread_descriptor(thread_descriptor_t *desc, const char **deps, int dep_count);
thread_descriptor_t *find_thread_descriptor(const char *name);
int start_thread_with_dependencies(const char *thread_name);
int start_all_auto_threads(void);
void print_thread_status(void);
bool check_thread_dependencies(const char *thread_name);

// Define a thread and its dependencies
#define THREAD_DEFINE(thread_name, init_function, ready_semaphore, ...)                            \
    static const char *thread_name##_deps[] = {__VA_ARGS__ __VA_OPT__(, ) NULL};               \
    static int thread_name##_dep_count = (sizeof(thread_name##_deps) / sizeof(char *)) - 1;    \
    static thread_descriptor_t thread_name##_desc = {.name = #thread_name,                     \
                             .init_func = init_function,               \
                             .ready_sem = &ready_semaphore,            \
                             .state = THREAD_STATE_INITIAL,            \
                             .dependency_count = 0,                    \
                             .auto_start = true};                      \
    static void __attribute__((constructor)) thread_name##_register(void)                      \
    {                                                                                          \
        register_thread_descriptor(&thread_name##_desc, thread_name##_deps,                \
                       thread_name##_dep_count);                               \
    }

// Define a thread without semaphore
#define THREAD_DEFINE_NO_SEM(thread_name, init_function, ...)                                      \
    static const char *thread_name##_deps[] = {__VA_ARGS__ __VA_OPT__(, ) NULL};               \
    static int thread_name##_dep_count = (sizeof(thread_name##_deps) / sizeof(char *)) - 1;    \
    static thread_descriptor_t thread_name##_desc = {.name = #thread_name,                     \
                             .init_func = init_function,               \
                             .ready_sem = NULL,                        \
                             .state = THREAD_STATE_INITIAL,            \
                             .dependency_count = 0,                    \
                             .auto_start = false};                     \
    static void __attribute__((constructor)) thread_name##_register(void)                      \
    {                                                                                          \
        register_thread_descriptor(&thread_name##_desc, thread_name##_deps,                \
                       thread_name##_dep_count);                               \
    }

// Start a thread (checking dependencies)
#define THREAD_START(thread_name) start_thread_with_dependencies(#thread_name)

#ifdef __cplusplus
}
#endif

#endif // THREAD_DEPENDENCY_MGR_HPP
