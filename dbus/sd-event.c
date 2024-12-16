#include "systemd/sd-event.h"
int sd_event_default(sd_event **e){}
sd_event* sd_event_ref(sd_event *e){}
sd_event* sd_event_unref(sd_event *e){}
sd_event_source* sd_event_source_unref(sd_event_source *s){}
int sd_event_add_time(sd_event *e, sd_event_source **s, clockid_t clock, uint64_t usec, uint64_t accuracy, sd_event_time_handler_t callback, void *userdata){}
int sd_event_wait(sd_event *e, uint64_t usec){}
int sd_event_dispatch(sd_event *e){}
int sd_event_loop(sd_event *e){}
int sd_event_exit(sd_event *e, int code){}
int sd_event_get_exit_code(sd_event *e, int *ret){}
int sd_event_get_watchdog(sd_event *e){}
sd_event_source* sd_event_source_ref(sd_event_source *s){}
void* sd_event_source_get_userdata(sd_event_source *s){}
void* sd_event_source_set_userdata(sd_event_source *s, void *userdata){}
int sd_event_add_child(sd_event *e, sd_event_source **s, pid_t pid, int options, sd_event_child_handler_t callback, void *userdata){}
int sd_event_add_defer(sd_event *e, sd_event_source **s, sd_event_handler_t callback, void *userdata){}
int sd_event_add_post(sd_event *e, sd_event_source **s, sd_event_handler_t callback, void *userdata){}
int sd_event_add_exit(sd_event *e, sd_event_source **s, sd_event_handler_t callback, void *userdata){}
int sd_event_prepare(sd_event *e){}
int sd_event_now(sd_event *e, clockid_t clock, uint64_t *ret){}
int sd_event_set_watchdog(sd_event *e, int b){}

int sd_event_source_set_description(sd_event_source *s, const char *description){}
int sd_event_source_get_description(sd_event_source *s, const char **ret){}
int sd_event_source_set_prepare(sd_event_source *s, sd_event_handler_t callback){}
int sd_event_source_get_pending(sd_event_source *s){}
int sd_event_source_get_priority(sd_event_source *s, int64_t *ret){}
int sd_event_source_set_priority(sd_event_source *s, int64_t priority){}
int sd_event_source_get_enabled(sd_event_source *s, int *ret){}
int sd_event_source_set_enabled(sd_event_source *s, int enabled){}
int sd_event_source_get_io_fd(sd_event_source *s){}
int sd_event_source_set_io_fd(sd_event_source *s, int fd){}
int sd_event_source_get_io_fd_own(sd_event_source *s){}
int sd_event_source_set_io_fd_own(sd_event_source *s, int own){}
int sd_event_source_get_io_events(sd_event_source *s, uint32_t *ret){}
int sd_event_source_set_io_events(sd_event_source *s, uint32_t events){}
int sd_event_source_get_io_revents(sd_event_source *s, uint32_t *ret){}
int sd_event_source_get_time(sd_event_source *s, uint64_t *ret){}
int sd_event_source_set_time(sd_event_source *s, uint64_t usec){}
int sd_event_source_get_time_accuracy(sd_event_source *s, uint64_t *ret){}
int sd_event_source_set_time_accuracy(sd_event_source *s, uint64_t usec){}
int sd_event_source_get_signal(sd_event_source *s){}
int sd_event_source_get_child_pid(sd_event_source *s, pid_t *ret){}
int sd_event_source_set_destroy_callback(sd_event_source *s, sd_event_destroy_t callback){}
int sd_event_source_get_destroy_callback(sd_event_source *s, sd_event_destroy_t *ret){}
int sd_event_source_get_floating(sd_event_source *s){}
int sd_event_source_set_floating(sd_event_source *s, int b){}
int sd_event_add_io(sd_event *e, sd_event_source **s, int fd, uint32_t events, sd_event_io_handler_t callback, void *userdata){}
int sd_event_add_signal(sd_event *e, sd_event_source **s, int sig, sd_event_signal_handler_t callback, void *userdata){}
int sd_event_new(sd_event **e){}
int sd_event_add_time_relative(sd_event *e, sd_event_source **s, clockid_t clock, uint64_t usec, uint64_t accuracy, sd_event_time_handler_t callback, void *userdata){}
int sd_event_run(sd_event *e, uint64_t usec){}
int sd_event_get_fd(sd_event *e){}
int sd_event_get_state(sd_event *e){}