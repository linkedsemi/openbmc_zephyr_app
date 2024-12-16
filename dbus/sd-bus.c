#include "systemd/sd-bus.h"
const unsigned sd_bus_object_vtable_format;

int sd_bus_default(sd_bus **ret){}
int sd_bus_default_user(sd_bus **ret){}
int sd_bus_default_system(sd_bus **ret){}
int sd_bus_open(sd_bus **ret){}
int sd_bus_open_user(sd_bus **ret){}
int sd_bus_open_system(sd_bus **ret){}


int sd_bus_process(sd_bus *bus, sd_bus_message **r){}
int sd_bus_get_timeout(sd_bus *bus, uint64_t *timeout_usec){}
int sd_bus_get_fd(sd_bus *bus){}
sd_bus_message* sd_bus_message_unref(sd_bus_message *m){}
sd_bus_message* sd_bus_message_ref(sd_bus_message *m){}
int sd_bus_add_match(sd_bus *bus, sd_bus_slot **slot, const char *match, sd_bus_message_handler_t callback, void *userdata){}
int sd_bus_emit_interfaces_added_strv(sd_bus *bus, const char *path, char **interfaces){}
int sd_bus_emit_interfaces_removed_strv(sd_bus *bus, const char *path, char **interfaces){}
void sd_bus_error_free(sd_bus_error *e){}
int sd_bus_error_set_errno(sd_bus_error *e, int error){}
int sd_bus_error_is_set(const sd_bus_error *e){}
int sd_bus_error_get_errno(const sd_bus_error *e){}
sd_bus_slot* sd_bus_slot_unref(sd_bus_slot *slot){}
sd_bus* sd_bus_ref(sd_bus *bus){}
int sd_bus_get_unique_name(sd_bus *bus, const char **unique){}
int sd_bus_add_object_vtable(sd_bus *bus, sd_bus_slot **slot, const char *path, const char *interface, const sd_bus_vtable *vtable, void *userdata){}
sd_bus* sd_bus_message_get_bus(sd_bus_message *m){}
int sd_bus_wait(sd_bus *bus, uint64_t timeout_usec){}
int sd_bus_is_open(sd_bus *bus){}
void sd_bus_close(sd_bus *bus){}
int sd_bus_flush(sd_bus *bus){}
sd_bus* sd_bus_flush_close_unref(sd_bus *bus){}
sd_bus* sd_bus_unref(sd_bus *bus){}
int sd_bus_send(sd_bus *bus, sd_bus_message *m, uint64_t *cookie){}
int sd_bus_request_name(sd_bus *bus, const char *name, uint64_t flags){}
void* sd_bus_slot_set_userdata(sd_bus_slot *slot, void *userdata){}
int sd_bus_slot_set_destroy_callback(sd_bus_slot *s, sd_bus_destroy_t callback){}
int sd_bus_message_verify_type(sd_bus_message *m, char type, const char *contents){}
int sd_bus_message_skip(sd_bus_message *m, const char *types){}
int sd_bus_message_read_basic(sd_bus_message *m, char type, void *p){}
int sd_bus_message_open_container(sd_bus_message *m, char type, const char *contents){}
int sd_bus_message_new_signal(sd_bus *bus, sd_bus_message **m, const char *path, const char *interface, const char *member){}
int sd_bus_message_new_method_errno(sd_bus_message *call, sd_bus_message **m, int error, const sd_bus_error *e){}
int sd_bus_message_new_method_errnof(sd_bus_message *call, sd_bus_message **m, int error, const char *format, ...){}
int sd_bus_message_new_method_return(sd_bus_message *call, sd_bus_message **m){}
int sd_bus_message_new_method_call(sd_bus *bus, sd_bus_message **m, const char *destination, const char *path, const char *interface, const char *member){}
int sd_bus_message_is_signal(sd_bus_message *m, const char *interface, const char *member){}
int sd_bus_message_is_method_error(sd_bus_message *m, const char *name){}
int sd_bus_message_is_method_call(sd_bus_message *m, const char *interface, const char *member){}
const sd_bus_error* sd_bus_message_get_error(sd_bus_message *m){}
const char* sd_bus_message_get_signature(sd_bus_message *m, int complete){}
const char* sd_bus_message_get_sender(sd_bus_message *m){}
const char* sd_bus_message_get_path(sd_bus_message *m){}
const char* sd_bus_message_get_member(sd_bus_message *m){}
const char* sd_bus_message_get_interface(sd_bus_message *m){}
const char* sd_bus_message_get_destination(sd_bus_message *m){}
int sd_bus_message_get_reply_cookie(sd_bus_message *m, uint64_t *cookie){}
int sd_bus_message_get_cookie(sd_bus_message *m, uint64_t *cookie){}
int sd_bus_message_get_type(sd_bus_message *m, uint8_t *type){}
int sd_bus_message_exit_container(sd_bus_message *m){}
int sd_bus_message_enter_container(sd_bus_message *m, char type, const char *contents){}
int sd_bus_message_close_container(sd_bus_message *m){}
int sd_bus_message_at_end(sd_bus_message *m, int complete){}
int sd_bus_message_append_string_iovec(sd_bus_message *m, const struct iovec *iov, unsigned n){}
int sd_bus_message_append_basic(sd_bus_message *m, char type, const void *p){}
int sd_bus_list_names(sd_bus *bus, char ***acquired, char ***activatable){} /* free the results */
sd_event* sd_bus_get_event(sd_bus *bus){}
int sd_bus_error_set_const(sd_bus_error *e, const char *name, const char *message){}
int sd_bus_error_set(sd_bus_error *e, const char *name, const char *message){}
int sd_bus_emit_properties_changed_strv(sd_bus *bus, const char *path, const char *interface, char **names){}
int sd_bus_emit_object_removed(sd_bus *bus, const char *path){}
int sd_bus_emit_object_added(sd_bus *bus, const char *path){}
int sd_bus_detach_event(sd_bus *bus){}
int sd_bus_call_async(sd_bus *bus, sd_bus_slot **slot, sd_bus_message *m, sd_bus_message_handler_t callback, void *userdata, uint64_t usec){}
int sd_bus_call(sd_bus *bus, sd_bus_message *m, uint64_t usec, sd_bus_error *ret_error, sd_bus_message **reply){}
int sd_bus_attach_event(sd_bus *bus, sd_event *e, int priority){}
int sd_bus_add_object_manager(sd_bus *bus, sd_bus_slot **slot, const char *path){}
int sd_bus_message_new_method_error(sd_bus_message *call, sd_bus_message **m, const sd_bus_error *e){}
int sd_bus_message_new_method_errorf(sd_bus_message *call, sd_bus_message **m, const char *name, const char *format, ...){}
int sd_bus_message_get_errno(sd_bus_message *m){}
int sd_bus_error_add_map(const sd_bus_error_map *map){}
int sd_bus_member_name_is_valid(const char *p){}
int sd_bus_message_append(sd_bus_message *m, const char *types, ...){}
int sd_id128_get_machine_app_specific(sd_id128_t app_id, sd_id128_t *ret){}
char* sd_id128_to_string(sd_id128_t id, char s[SD_ID128_STRING_MAX]){}