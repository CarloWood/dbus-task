#pragma once
#include <systemd/sd-bus.h>

#ifndef SB_BUS_NO_WRAP
#define sd_bus_add_object wrap_bus_add_object
#define sd_bus_call_async wrap_bus_call_async
#define sd_bus_error_copy wrap_bus_error_copy
#define sd_bus_error_get_errno wrap_bus_error_get_errno
#define sd_bus_error_is_set wrap_bus_error_is_set
#define sd_bus_error_move wrap_bus_error_move
#define sd_bus_error_set wrap_bus_error_set
#define sd_bus_error_set_errno wrap_bus_error_set_errno
#define sd_bus_error_set_errnofv wrap_bus_error_set_errnofv
#define sd_bus_get_events wrap_bus_get_events
#define sd_bus_get_fd wrap_bus_get_fd
#define sd_bus_get_unique_name wrap_bus_get_unique_name
#define sd_bus_match_signal_async wrap_bus_match_signal_async
#define sd_bus_message_append_array wrap_bus_message_append_array
#define sd_bus_message_append_basic wrap_bus_message_append_basic
#define sd_bus_message_at_end wrap_bus_message_at_end
#define sd_bus_message_enter_container wrap_bus_message_enter_container
#define sd_bus_message_exit_container wrap_bus_message_exit_container
#define sd_bus_message_get_allow_interactive_authorization wrap_bus_message_get_allow_interactive_authorization
#define sd_bus_message_get_auto_start wrap_bus_message_get_auto_start
#define sd_bus_message_get_bus wrap_bus_message_get_bus
#define sd_bus_message_get_cookie wrap_bus_message_get_cookie
#define sd_bus_message_get_destination wrap_bus_message_get_destination
#define sd_bus_message_get_error wrap_bus_message_get_error
#define sd_bus_message_get_expect_reply wrap_bus_message_get_expect_reply
#define sd_bus_message_get_interface wrap_bus_message_get_interface
#define sd_bus_message_get_member wrap_bus_message_get_member
#define sd_bus_message_get_path wrap_bus_message_get_path
#define sd_bus_message_get_reply_cookie wrap_bus_message_get_reply_cookie
#define sd_bus_message_get_sender wrap_bus_message_get_sender
#define sd_bus_message_get_signature wrap_bus_message_get_signature
#define sd_bus_message_get_type wrap_bus_message_get_type
#define sd_bus_message_has_signature wrap_bus_message_has_signature
#define sd_bus_message_is_empty wrap_bus_message_is_empty
#define sd_bus_message_is_method_call wrap_bus_message_is_method_call
#define sd_bus_message_is_method_error wrap_bus_message_is_method_error
#define sd_bus_message_is_signal wrap_bus_message_is_signal
#define sd_bus_message_new_method_call wrap_bus_message_new_method_call
#define sd_bus_message_peek_type wrap_bus_message_peek_type
#define sd_bus_message_ref wrap_bus_message_ref
#define sd_bus_message_unref wrap_bus_message_unref
#define sd_bus_open_system_with_description wrap_bus_open_system_with_description
#define sd_bus_open_user_with_description wrap_bus_open_user_with_description
#define sd_bus_process wrap_bus_process
#define sd_bus_request_name_async wrap_bus_request_name_async
#define sd_bus_slot_unref wrap_bus_slot_unref
#define sd_bus_error_free wrap_bus_error_free
#define sd_bus_message_read wrap_bus_message_read
#define sd_bus_reply_method_return wrap_bus_reply_method_return
#endif

#define SD_BUS_FOREACH_NON_VOID_FUNCTION(X) \
  X(int, bus_add_object, (sd_bus* bus, sd_bus_slot** slot, char const* path, sd_bus_message_handler_t callback, void* userdata), bus, slot, path, callback, userdata) \
  X(int, bus_call_async, (sd_bus* bus, sd_bus_slot** slot, sd_bus_message* m, sd_bus_message_handler_t callback, void* userdata, uint64_t usec), bus, slot, m, callback, userdata, usec) \
  X(int, bus_error_copy, (sd_bus_error* dest, sd_bus_error const* e), dest, e) \
  X(int, bus_error_get_errno, (sd_bus_error const* e), e) \
  X(int, bus_error_is_set, (sd_bus_error const* e), e) \
  X(int, bus_error_move, (sd_bus_error* dest, sd_bus_error* e), dest, e) \
  X(int, bus_error_set, (sd_bus_error* e, char const* name, char const* message), e, name, message) \
  X(int, bus_error_set_errno, (sd_bus_error* e, int error), e, error) \
  X(int, bus_error_set_errnofv, (sd_bus_error* e, int error, char const* format, va_list ap), e, error, format, ap) \
  X(int, bus_get_events, (sd_bus* bus), bus) \
  X(int, bus_get_fd, (sd_bus* bus), bus) \
  X(int, bus_get_unique_name, (sd_bus* bus, char const** unique), bus, unique) \
  X(int, bus_match_signal_async, \
      (sd_bus* bus, sd_bus_slot** ret, char const* sender, char const* path, char const* interface, char const* member, \
       sd_bus_message_handler_t callback, sd_bus_message_handler_t install_callback, void* userdata), \
      bus, ret, sender, path, interface, member, callback, install_callback, userdata) \
  X(int, bus_message_append_array, (sd_bus_message* m, char type, void const* ptr, size_t size), m, type, ptr, size) \
  X(int, bus_message_append_basic, (sd_bus_message* m, char type, void const* p), m, type, p) \
  X(int, bus_message_at_end, (sd_bus_message* m, int complete), m, complete) \
  X(int, bus_message_enter_container, (sd_bus_message* m, char type, char const* contents), m, type, contents) \
  X(int, bus_message_exit_container, (sd_bus_message* m), m) \
  X(int, bus_message_get_allow_interactive_authorization, (sd_bus_message* m), m) \
  X(int, bus_message_get_auto_start, (sd_bus_message* m), m) \
  X(sd_bus*, bus_message_get_bus, (sd_bus_message* m), m) \
  X(int, bus_message_get_cookie, (sd_bus_message* m, uint64_t* cookie), m, cookie) \
  X(char const*, bus_message_get_destination, (sd_bus_message* m), m) \
  X(sd_bus_error const*, bus_message_get_error, (sd_bus_message* m), m) \
  X(int, bus_message_get_expect_reply, (sd_bus_message* m), m) \
  X(char const*, bus_message_get_interface, (sd_bus_message* m), m) \
  X(char const*, bus_message_get_member, (sd_bus_message* m), m) \
  X(char const*, bus_message_get_path, (sd_bus_message* m), m) \
  X(int, bus_message_get_reply_cookie, (sd_bus_message* m, uint64_t* cookie), m, cookie) \
  X(char const*, bus_message_get_sender, (sd_bus_message* m), m) \
  X(char const*, bus_message_get_signature, (sd_bus_message* m, int complete), m, complete) \
  X(int, bus_message_get_type, (sd_bus_message* m, uint8_t* type), m, type) \
  X(int, bus_message_has_signature, (sd_bus_message* m, char const* signature), m, signature) \
  X(int, bus_message_is_empty, (sd_bus_message* m), m) \
  X(int, bus_message_is_method_call, (sd_bus_message* m, char const* interface, char const* member), m, interface, member) \
  X(int, bus_message_is_method_error, (sd_bus_message* m, char const* name), m, name) \
  X(int, bus_message_is_signal, (sd_bus_message* m, char const* interface, char const* member), m, interface, member) \
  X(int, bus_message_new_method_call, \
      (sd_bus* bus, sd_bus_message** m, char const* destination, char const* path, char const* interface, char const* member), \
      bus, m, destination, path, interface, member) \
  X(int, bus_message_peek_type, (sd_bus_message* m, char* type, char const** contents), m, type, contents) \
  X(sd_bus_message*, bus_message_ref, (sd_bus_message* m), m) \
  X(sd_bus_message*, bus_message_unref, (sd_bus_message* m), m) \
  X(int, bus_open_system_with_description, (sd_bus** ret, char const* description), ret, description) \
  X(int, bus_open_user_with_description, (sd_bus** ret, char const* description), ret, description) \
  X(int, bus_process, (sd_bus* bus, sd_bus_message* *ret), bus, ret) \
  X(int, bus_request_name_async, \
      (sd_bus* bus, sd_bus_slot** ret_slot, char const* name, uint64_t flags, sd_bus_message_handler_t callback, void* userdata), \
      bus, ret_slot, name, flags, callback, userdata) \
  X(sd_bus_slot*, bus_slot_unref, (sd_bus_slot* slot), slot)

#define SD_BUS_FOREACH_VOID_FUNCTION(X) \
  X(void, bus_error_free, (sd_bus_error* e), (e))

#define SD_BUS_FOREACH_ELIPSIS_FUNCTION(X) \
  X(int, bus_message_read, (sd_bus_message* m, char const* types, ...), m, types) \
  X(int, bus_reply_method_return, (sd_bus_message* call, char const* types, ...), call, types)

#define SD_BUS_DECLARE(R, N, P, ...) \
  R wrap_##N P;

SD_BUS_FOREACH_VOID_FUNCTION(SD_BUS_DECLARE)
SD_BUS_FOREACH_NON_VOID_FUNCTION(SD_BUS_DECLARE)
SD_BUS_FOREACH_ELIPSIS_FUNCTION(SD_BUS_DECLARE)

#include "OneThreadAtATime.h"
extern OneThreadAtATime dbus_critical_area;
