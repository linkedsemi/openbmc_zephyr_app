#ifndef SD_BUS_H_
#define SD_BUS_H_
#include <stdint.h>
#include <zephyr/net/net_ip.h>
#include "sd-event.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {
        _SD_BUS_VTABLE_PARAM_NAMES     = 1 << 0
};
enum {
        _SD_BUS_MESSAGE_TYPE_INVALID = 0,
        SD_BUS_MESSAGE_METHOD_CALL,
        SD_BUS_MESSAGE_METHOD_RETURN,
        SD_BUS_MESSAGE_METHOD_ERROR,
        SD_BUS_MESSAGE_SIGNAL,
        _SD_BUS_MESSAGE_TYPE_MAX
};
enum {
        _SD_BUS_VTABLE_START             = '<',
        _SD_BUS_VTABLE_END               = '>',
        _SD_BUS_VTABLE_METHOD            = 'M',
        _SD_BUS_VTABLE_SIGNAL            = 'S',
        _SD_BUS_VTABLE_PROPERTY          = 'P',
        _SD_BUS_VTABLE_WRITABLE_PROPERTY = 'W'
};
enum {
        _SD_BUS_TYPE_INVALID         = 0,
        SD_BUS_TYPE_BYTE             = 'y',
        SD_BUS_TYPE_BOOLEAN          = 'b',
        SD_BUS_TYPE_INT16            = 'n',
        SD_BUS_TYPE_UINT16           = 'q',
        SD_BUS_TYPE_INT32            = 'i',
        SD_BUS_TYPE_UINT32           = 'u',
        SD_BUS_TYPE_INT64            = 'x',
        SD_BUS_TYPE_UINT64           = 't',
        SD_BUS_TYPE_DOUBLE           = 'd',
        SD_BUS_TYPE_STRING           = 's',
        SD_BUS_TYPE_OBJECT_PATH      = 'o',
        SD_BUS_TYPE_SIGNATURE        = 'g',
        SD_BUS_TYPE_UNIX_FD          = 'h',
        SD_BUS_TYPE_ARRAY            = 'a',
        SD_BUS_TYPE_VARIANT          = 'v',
        SD_BUS_TYPE_STRUCT           = 'r', /* not actually used in signatures */
        SD_BUS_TYPE_STRUCT_BEGIN     = '(',
        SD_BUS_TYPE_STRUCT_END       = ')',
        SD_BUS_TYPE_DICT_ENTRY       = 'e', /* not actually used in signatures */
        SD_BUS_TYPE_DICT_ENTRY_BEGIN = '{',
        SD_BUS_TYPE_DICT_ENTRY_END   = '}'
};
enum {
        SD_BUS_NAME_REPLACE_EXISTING  = 1ULL << 0,
        SD_BUS_NAME_ALLOW_REPLACEMENT = 1ULL << 1,
        SD_BUS_NAME_QUEUE             = 1ULL << 2
};
enum {
        SD_BUS_VTABLE_DEPRECATED                   = 1ULL << 0,
        SD_BUS_VTABLE_HIDDEN                       = 1ULL << 1,
        SD_BUS_VTABLE_UNPRIVILEGED                 = 1ULL << 2,
        SD_BUS_VTABLE_METHOD_NO_REPLY              = 1ULL << 3,
        SD_BUS_VTABLE_PROPERTY_CONST               = 1ULL << 4,
        SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE        = 1ULL << 5,
        SD_BUS_VTABLE_PROPERTY_EMITS_INVALIDATION  = 1ULL << 6,
        SD_BUS_VTABLE_PROPERTY_EXPLICIT            = 1ULL << 7,
        SD_BUS_VTABLE_SENSITIVE                    = 1ULL << 8, /* covers both directions: method call + reply */
        SD_BUS_VTABLE_ABSOLUTE_OFFSET              = 1ULL << 9,
        _SD_BUS_VTABLE_CAPABILITY_MASK             = 0xFFFFULL << 40
};


#define SD_BUS_ERROR_FAILED                             "org.freedesktop.DBus.Error.Failed"
#define SD_BUS_ERROR_NO_MEMORY                          "org.freedesktop.DBus.Error.NoMemory"
#define SD_BUS_ERROR_SERVICE_UNKNOWN                    "org.freedesktop.DBus.Error.ServiceUnknown"
#define SD_BUS_ERROR_NAME_HAS_NO_OWNER                  "org.freedesktop.DBus.Error.NameHasNoOwner"
#define SD_BUS_ERROR_NO_REPLY                           "org.freedesktop.DBus.Error.NoReply"
#define SD_BUS_ERROR_IO_ERROR                           "org.freedesktop.DBus.Error.IOError"
#define SD_BUS_ERROR_BAD_ADDRESS                        "org.freedesktop.DBus.Error.BadAddress"
#define SD_BUS_ERROR_NOT_SUPPORTED                      "org.freedesktop.DBus.Error.NotSupported"
#define SD_BUS_ERROR_LIMITS_EXCEEDED                    "org.freedesktop.DBus.Error.LimitsExceeded"
#define SD_BUS_ERROR_ACCESS_DENIED                      "org.freedesktop.DBus.Error.AccessDenied"
#define SD_BUS_ERROR_AUTH_FAILED                        "org.freedesktop.DBus.Error.AuthFailed"
#define SD_BUS_ERROR_NO_SERVER                          "org.freedesktop.DBus.Error.NoServer"
#define SD_BUS_ERROR_TIMEOUT                            "org.freedesktop.DBus.Error.Timeout"
#define SD_BUS_ERROR_NO_NETWORK                         "org.freedesktop.DBus.Error.NoNetwork"
#define SD_BUS_ERROR_ADDRESS_IN_USE                     "org.freedesktop.DBus.Error.AddressInUse"
#define SD_BUS_ERROR_DISCONNECTED                       "org.freedesktop.DBus.Error.Disconnected"
#define SD_BUS_ERROR_INVALID_ARGS                       "org.freedesktop.DBus.Error.InvalidArgs"
#define SD_BUS_ERROR_FILE_NOT_FOUND                     "org.freedesktop.DBus.Error.FileNotFound"
#define SD_BUS_ERROR_FILE_EXISTS                        "org.freedesktop.DBus.Error.FileExists"
#define SD_BUS_ERROR_UNKNOWN_METHOD                     "org.freedesktop.DBus.Error.UnknownMethod"
#define SD_BUS_ERROR_UNKNOWN_OBJECT                     "org.freedesktop.DBus.Error.UnknownObject"
#define SD_BUS_ERROR_UNKNOWN_INTERFACE                  "org.freedesktop.DBus.Error.UnknownInterface"
#define SD_BUS_ERROR_UNKNOWN_PROPERTY                   "org.freedesktop.DBus.Error.UnknownProperty"
#define SD_BUS_ERROR_PROPERTY_READ_ONLY                 "org.freedesktop.DBus.Error.PropertyReadOnly"
#define SD_BUS_ERROR_UNIX_PROCESS_ID_UNKNOWN            "org.freedesktop.DBus.Error.UnixProcessIdUnknown"
#define SD_BUS_ERROR_INVALID_SIGNATURE                  "org.freedesktop.DBus.Error.InvalidSignature"
#define SD_BUS_ERROR_INCONSISTENT_MESSAGE               "org.freedesktop.DBus.Error.InconsistentMessage"
#define SD_BUS_ERROR_TIMED_OUT                          "org.freedesktop.DBus.Error.TimedOut"
#define SD_BUS_ERROR_MATCH_RULE_NOT_FOUND               "org.freedesktop.DBus.Error.MatchRuleNotFound"
#define SD_BUS_ERROR_MATCH_RULE_INVALID                 "org.freedesktop.DBus.Error.MatchRuleInvalid"
#define SD_BUS_ERROR_INTERACTIVE_AUTHORIZATION_REQUIRED "org.freedesktop.DBus.Error.InteractiveAuthorizationRequired"
#define SD_BUS_ERROR_INVALID_FILE_CONTENT               "org.freedesktop.DBus.Error.InvalidFileContent"
#define SD_BUS_ERROR_SELINUX_SECURITY_CONTEXT_UNKNOWN   "org.freedesktop.DBus.Error.SELinuxSecurityContextUnknown"
#define SD_BUS_ERROR_OBJECT_PATH_IN_USE                 "org.freedesktop.DBus.Error.ObjectPathInUse"

#define SD_BUS_VTABLE_START(_flags)                                     \
        {                                                               \
                .type = _SD_BUS_VTABLE_START,                           \
                .flags = _flags,                                        \
                .x = {                                                  \
                        .start = {                                      \
                                .element_size = sizeof(sd_bus_vtable),  \
                                .features = _SD_BUS_VTABLE_PARAM_NAMES, \
                                .vtable_format_reference = &sd_bus_object_vtable_format, \
                        },                                              \
                },                                                      \
        }

#define SD_BUS_VTABLE_END                                               \
        {                                                               \
                .type = _SD_BUS_VTABLE_END,                             \
                .flags = 0,                                             \
                .x = {                                                  \
                        .end = {                                        \
                                ._reserved = 0,                         \
                        },                                              \
                },                                                      \
        }
#define SD_BUS_METHOD_WITH_NAMES_OFFSET(_member, _signature, _in_names, _result, _out_names, _handler, _offset, _flags)  \
        {                                                               \
                .type = _SD_BUS_VTABLE_METHOD,                          \
                .flags = _flags,                                        \
                .x = {                                                  \
                        .method = {                                     \
                                .member = _member,                      \
                                .signature = _signature,                \
                                .result = _result,                      \
                                .handler = _handler,                    \
                                .offset = _offset,                      \
                                .names = _in_names _out_names,          \
                        },                                              \
                },                                                      \
        }
#define SD_BUS_METHOD_WITH_OFFSET(_member, _signature, _result, _handler, _offset, _flags)   \
        SD_BUS_METHOD_WITH_NAMES_OFFSET(_member, _signature, "", _result, "", _handler, _offset, _flags)
#define SD_BUS_METHOD_WITH_NAMES(_member, _signature, _in_names, _result, _out_names, _handler, _flags)   \
        SD_BUS_METHOD_WITH_NAMES_OFFSET(_member, _signature, _in_names, _result, _out_names, _handler, 0, _flags)
#define SD_BUS_METHOD(_member, _signature, _result, _handler, _flags)   \
        SD_BUS_METHOD_WITH_NAMES_OFFSET(_member, _signature, "", _result, "", _handler, 0, _flags)
#define SD_BUS_ERROR_MAKE_CONST(name, message) ((const sd_bus_error) {(name), (message), 0})
#define SD_BUS_ERROR_NULL SD_BUS_ERROR_MAKE_CONST(NULL, NULL)

#define SD_BUS_SIGNAL_WITH_NAMES(_member, _signature, _out_names, _flags)                      \
        {                                                               \
                .type = _SD_BUS_VTABLE_SIGNAL,                          \
                .flags = _flags,                                        \
                .x = {                                                  \
                        .signal = {                                     \
                                .member = _member,                      \
                                .signature = _signature,                \
                                .names = _out_names,                    \
                        },                                              \
                },                                                      \
                        }
#define SD_BUS_SIGNAL(_member, _signature, _flags)                      \
        SD_BUS_SIGNAL_WITH_NAMES(_member, _signature, "", _flags)

#define SD_BUS_PROPERTY(_member, _signature, _get, _offset, _flags)     \
        {                                                               \
                .type = _SD_BUS_VTABLE_PROPERTY,                        \
                .flags = _flags,                                        \
                .x = {                                                  \
                        .property = {                                   \
                                .member = _member,                      \
                                .signature = _signature,                \
                                .get = _get,                            \
                                .set = NULL,                            \
                                .offset = _offset,                      \
                        },                                              \
                },                                                      \
        }
#define SD_BUS_WRITABLE_PROPERTY(_member, _signature, _get, _set, _offset, _flags) \
        {                                                               \
                .type = _SD_BUS_VTABLE_WRITABLE_PROPERTY,               \
                .flags = _flags,                                        \
                .x = {                                                  \
                        .property = {                                   \
                                .member = _member,                      \
                                .signature = _signature,                \
                                .get = _get,                            \
                                .set = _set,                            \
                                .offset = _offset,                      \
                        },                                              \
                },                                                      \
        }
#define SD_BUS_ERROR_MAP(_name, _code)          \
        {                                       \
                .name = _name,                  \
                .code = _code,                  \
        }
#define SD_BUS_ERROR_MAP_END                    \
        {                                       \
                .name = NULL,                   \
                .code = - 'x',                  \
        }

#define SD_ID128_ARRAY(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15) \
        { .bytes = { 0x##v0, 0x##v1, 0x##v2, 0x##v3, 0x##v4, 0x##v5, 0x##v6, 0x##v7, \
                     0x##v8, 0x##v9, 0x##v10, 0x##v11, 0x##v12, 0x##v13, 0x##v14, 0x##v15 }}

#define SD_ID128_MAKE(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15) \
        ((const sd_id128_t) SD_ID128_ARRAY(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15))
        
#define SD_ID128_STRING_MAX 33U
#define SD_ID128_UUID_STRING_MAX 37U

typedef struct sd_bus sd_bus;
typedef struct sd_bus_message sd_bus_message;
typedef struct sd_bus_slot sd_bus_slot;
typedef struct sd_bus_creds sd_bus_creds;
typedef struct sd_bus_track sd_bus_track;
typedef union sd_id128 sd_id128_t;

union sd_id128 {
        uint8_t bytes[16];
        uint64_t qwords[2];
};

typedef struct {
        const char *name;
        const char *message;
        int _need_free;
} sd_bus_error;
typedef struct {
        const char *name;
        int code;
} sd_bus_error_map;

typedef int (*sd_bus_message_handler_t)(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
typedef int (*sd_bus_property_get_t) (sd_bus *bus, const char *path, const char *interface, const char *property, sd_bus_message *reply, void *userdata, sd_bus_error *ret_error);
typedef int (*sd_bus_property_set_t) (sd_bus *bus, const char *path, const char *interface, const char *property, sd_bus_message *value, void *userdata, sd_bus_error *ret_error);
typedef int (*sd_bus_object_find_t) (sd_bus *bus, const char *path, const char *interface, void *userdata, void **ret_found, sd_bus_error *ret_error);
typedef int (*sd_bus_node_enumerator_t) (sd_bus *bus, const char *prefix, void *userdata, char ***ret_nodes, sd_bus_error *ret_error);
typedef int (*sd_bus_track_handler_t) (sd_bus_track *track, void *userdata);
typedef _sd_destroy_t sd_bus_destroy_t;
struct sd_bus_vtable {
        /* Please do not initialize this structure directly, use the
         * macros below instead */

        uint8_t type:8;
        uint64_t flags:56;
        union {
                struct {
                        size_t element_size;
                        uint64_t features;
                        const unsigned *vtable_format_reference;
                } start;
                struct {
                        /* This field exists only to make sure we have something to initialize in
                         * SD_BUS_VTABLE_END in a way that is both compatible with pedantic versions of C and
                         * C++. It's unused otherwise. */
                        size_t _reserved;
                } end;
                struct {
                        const char *member;
                        const char *signature;
                        const char *result;
                        sd_bus_message_handler_t handler;
                        size_t offset;
                        const char *names;
                } method;
                struct {
                        const char *member;
                        const char *signature;
                        const char *names;
                } signal;
                struct {
                        const char *member;
                        const char *signature;
                        sd_bus_property_get_t get;
                        sd_bus_property_set_t set;
                        size_t offset;
                } property;
        } x;
};
typedef struct sd_bus_vtable sd_bus_vtable;

extern const unsigned sd_bus_object_vtable_format;

int sd_bus_default(sd_bus **ret);
int sd_bus_default_user(sd_bus **ret);
int sd_bus_default_system(sd_bus **ret);
int sd_bus_open(sd_bus **ret);
int sd_bus_open_user(sd_bus **ret);
int sd_bus_open_system(sd_bus **ret);


int sd_bus_process(sd_bus *bus, sd_bus_message **r);
int sd_bus_get_timeout(sd_bus *bus, uint64_t *timeout_usec);
int sd_bus_get_fd(sd_bus *bus);
sd_bus_message* sd_bus_message_unref(sd_bus_message *m);
sd_bus_message* sd_bus_message_ref(sd_bus_message *m);
int sd_bus_add_match(sd_bus *bus, sd_bus_slot **slot, const char *match, sd_bus_message_handler_t callback, void *userdata);
int sd_bus_emit_interfaces_added_strv(sd_bus *bus, const char *path, char **interfaces);
int sd_bus_emit_interfaces_removed_strv(sd_bus *bus, const char *path, char **interfaces);
void sd_bus_error_free(sd_bus_error *e);
int sd_bus_error_set_errno(sd_bus_error *e, int error);
int sd_bus_error_is_set(const sd_bus_error *e);
int sd_bus_error_get_errno(const sd_bus_error *e);
sd_bus_slot* sd_bus_slot_unref(sd_bus_slot *slot);
sd_bus* sd_bus_ref(sd_bus *bus);
int sd_bus_get_unique_name(sd_bus *bus, const char **unique);
int sd_bus_add_object_vtable(sd_bus *bus, sd_bus_slot **slot, const char *path, const char *interface, const sd_bus_vtable *vtable, void *userdata);
sd_bus* sd_bus_message_get_bus(sd_bus_message *m);
int sd_bus_wait(sd_bus *bus, uint64_t timeout_usec);
int sd_bus_is_open(sd_bus *bus);
void sd_bus_close(sd_bus *bus);
int sd_bus_flush(sd_bus *bus);
sd_bus* sd_bus_flush_close_unref(sd_bus *bus);
sd_bus* sd_bus_unref(sd_bus *bus);
int sd_bus_send(sd_bus *bus, sd_bus_message *m, uint64_t *cookie);
int sd_bus_request_name(sd_bus *bus, const char *name, uint64_t flags);
void* sd_bus_slot_set_userdata(sd_bus_slot *slot, void *userdata);
int sd_bus_slot_set_destroy_callback(sd_bus_slot *s, sd_bus_destroy_t callback);
int sd_bus_message_verify_type(sd_bus_message *m, char type, const char *contents);
int sd_bus_message_skip(sd_bus_message *m, const char *types);
int sd_bus_message_read_basic(sd_bus_message *m, char type, void *p);
int sd_bus_message_open_container(sd_bus_message *m, char type, const char *contents);
int sd_bus_message_new_signal(sd_bus *bus, sd_bus_message **m, const char *path, const char *interface, const char *member);
int sd_bus_message_new_method_errno(sd_bus_message *call, sd_bus_message **m, int error, const sd_bus_error *e);
int sd_bus_message_new_method_errnof(sd_bus_message *call, sd_bus_message **m, int error, const char *format, ...);
int sd_bus_message_new_method_return(sd_bus_message *call, sd_bus_message **m);
int sd_bus_message_new_method_call(sd_bus *bus, sd_bus_message **m, const char *destination, const char *path, const char *interface, const char *member);
int sd_bus_message_is_signal(sd_bus_message *m, const char *interface, const char *member);
int sd_bus_message_is_method_error(sd_bus_message *m, const char *name);
int sd_bus_message_is_method_call(sd_bus_message *m, const char *interface, const char *member);
const sd_bus_error* sd_bus_message_get_error(sd_bus_message *m);
const char* sd_bus_message_get_signature(sd_bus_message *m, int complete);
const char* sd_bus_message_get_sender(sd_bus_message *m);
const char* sd_bus_message_get_path(sd_bus_message *m);
const char* sd_bus_message_get_member(sd_bus_message *m);
const char* sd_bus_message_get_interface(sd_bus_message *m);
const char* sd_bus_message_get_destination(sd_bus_message *m);
int sd_bus_message_get_reply_cookie(sd_bus_message *m, uint64_t *cookie);
int sd_bus_message_get_cookie(sd_bus_message *m, uint64_t *cookie);
int sd_bus_message_get_type(sd_bus_message *m, uint8_t *type);
int sd_bus_message_exit_container(sd_bus_message *m);
int sd_bus_message_enter_container(sd_bus_message *m, char type, const char *contents);
int sd_bus_message_close_container(sd_bus_message *m);
int sd_bus_message_at_end(sd_bus_message *m, int complete);
int sd_bus_message_append_string_iovec(sd_bus_message *m, const struct iovec *iov, unsigned n);
int sd_bus_message_append_basic(sd_bus_message *m, char type, const void *p);
int sd_bus_list_names(sd_bus *bus, char ***acquired, char ***activatable); /* free the results */
sd_event* sd_bus_get_event(sd_bus *bus);
int sd_bus_error_set_const(sd_bus_error *e, const char *name, const char *message);
int sd_bus_error_set(sd_bus_error *e, const char *name, const char *message);
int sd_bus_emit_properties_changed_strv(sd_bus *bus, const char *path, const char *interface, char **names);
int sd_bus_emit_object_removed(sd_bus *bus, const char *path);
int sd_bus_emit_object_added(sd_bus *bus, const char *path);
int sd_bus_detach_event(sd_bus *bus);
int sd_bus_call_async(sd_bus *bus, sd_bus_slot **slot, sd_bus_message *m, sd_bus_message_handler_t callback, void *userdata, uint64_t usec);
int sd_bus_call(sd_bus *bus, sd_bus_message *m, uint64_t usec, sd_bus_error *ret_error, sd_bus_message **reply);
int sd_bus_attach_event(sd_bus *bus, sd_event *e, int priority);
int sd_bus_add_object_manager(sd_bus *bus, sd_bus_slot **slot, const char *path);
int sd_bus_message_new_method_error(sd_bus_message *call, sd_bus_message **m, const sd_bus_error *e);
int sd_bus_message_new_method_errorf(sd_bus_message *call, sd_bus_message **m, const char *name, const char *format, ...);
int sd_bus_message_get_errno(sd_bus_message *m);
int sd_bus_error_add_map(const sd_bus_error_map *map);
int sd_bus_member_name_is_valid(const char *p);
int sd_bus_message_append(sd_bus_message *m, const char *types, ...);
int sd_id128_get_machine_app_specific(sd_id128_t app_id, sd_id128_t *ret);
char* sd_id128_to_string(sd_id128_t id, char s[SD_ID128_STRING_MAX]);

#ifdef __cplusplus
}
#endif
#endif