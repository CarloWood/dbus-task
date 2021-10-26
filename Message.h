#pragma once

#include "DBusConnection.h"
#include "Destination.h"
#include <boost/intrusive_ptr.hpp>
#include "systemd_sd-bus.h"
#include <iterator>
#include <algorithm>
#include <iterator>
#include "debug.h"

namespace dbus {

class MessageConst
{
 protected:
  sd_bus_message* m_message;
  sd_bus* m_bus;

 public:
  // Do not call any of the methods on a default constructed object. The result is UB.
  // I could have added an ASSERT(m_message); to every method, but decided not to.
  // Instead an ASSERT was added to constructor and assignment operator from a
  // sd_bus_message const*.
  MessageConst() : m_message(nullptr), m_bus((sd_bus*)0x33333333) { }

  // This isn't safe; please try to use the move constructor instead.
  MessageConst(MessageConst const&) = delete;

  // Construct a MessageConst from a sd_bus_message pointer.
  explicit MessageConst(sd_bus_message const* message, sd_bus* bus) : m_message(const_cast<sd_bus_message*>(message)), m_bus(bus)
  {
    // Do not pass nullptr to this constructor.
    ASSERT(message);
    sd_bus_message_ref(m_message);
  }

  // Move constructor.
  MessageConst(MessageConst&& message) : m_message(message.m_message)
  {
    message.m_message = nullptr;
  }

  // Destructor.
  ~MessageConst()
  {
    reset();
  }

  void reset()
  {
    if (m_message)
    {
      // An unref on a message is also an unref on the bus connection.
      // To unref the bus we need a lock. Therefore the message should
      // already be unref-ed in the finish_impl of the task that owns
      // this message. Such task should always exist because otherwise
      // the message can't have a bus set.
      sd_bus_message_unref(m_message);
      // Don't do that again upon destruction.
      m_message = nullptr;
    }
  }

  // Assignment operator.
  MessageConst& operator=(sd_bus_message const* message)
  {
    // Do not pass nullptr to the assignment operator.
    ASSERT(message);
    sd_bus_message_unref(m_message);
    m_message = const_cast<sd_bus_message*>(message);
    sd_bus_message_ref(m_message);
    return *this;
  }

  // Move assignment operator.
  MessageConst& operator=(MessageConst&& message)
  {
    m_message = message.m_message;
    message.m_message = nullptr;
    return *this;
  }

  operator sd_bus_message const*() const { return m_message; }

  sd_bus* get_bus() const { return sd_bus_message_get_bus(m_message); }
  bool is_signal(char const* interface = nullptr, char const* member = nullptr) const
  {
    return sd_bus_message_is_signal(m_message, interface, member);
  }
  bool is_method_call(char const* interface = nullptr, char const* member = nullptr) const
  {
    return sd_bus_message_is_method_call(m_message, interface, member);
  }
  bool is_method_error(char const* name = nullptr) const
  {
    return sd_bus_message_is_method_error(m_message, name);
  }

  // Possible return values:
  // SD_BUS_MESSAGE_METHOD_CALL, SD_BUS_MESSAGE_METHOD_RETURN, SD_BUS_MESSAGE_METHOD_ERROR, SD_BUS_MESSAGE_SIGNAL.
  uint8_t get_type() const
  {
    uint8_t type;
    sd_bus_message_get_type(m_message, &type); return type;
  }

  // The returned sd_bus_error has a life time equal to this MessageConst.
  sd_bus_error const* get_error() const
  {
    return sd_bus_message_get_error(m_message);
  }

  uint64_t get_reply_cookie() const
  {
    uint64_t cookie = 0;
    // Only call this function when get_type() returned SD_BUS_MESSAGE_METHOD_RETURN.
    ASSERT(get_type() == SD_BUS_MESSAGE_METHOD_RETURN);
    sd_bus_message_get_reply_cookie(m_message, &cookie);
    return cookie;
  }

  uint64_t get_cookie() const
  {
    uint64_t cookie = 0;
    sd_bus_message_get_cookie(m_message, &cookie);
    return cookie;
  }

  char const* get_sender() const
  {
    return sd_bus_message_get_sender(m_message);
  }
  char const* get_destination() const
  {
    return sd_bus_message_get_destination(m_message);
  }
  char const* get_path() const
  {
    return sd_bus_message_get_path(m_message);
  }
  char const* get_interface() const
  {
    return sd_bus_message_get_interface(m_message);
  }
  char const* get_member() const
  {
    return sd_bus_message_get_member(m_message);
  }

  // Will return false when type() isn't SD_BUS_MESSAGE_METHOD_CALL.
  // Otherwise returns false iff the NO_REPLY_EXPECTED flag is set on the message.
  bool get_expect_reply() const
  {
    return sd_bus_message_get_expect_reply(m_message);
  }

  // Will return false when type() isn't SD_BUS_MESSAGE_METHOD_CALL.
  // Otherwise returns true iff the ALLOW_INTERACTIVE_AUTHORIZATION flag is set on the message.
  bool get_allow_interactive_authorization() const
  {
    return sd_bus_message_get_allow_interactive_authorization(m_message);
  }

  // Returns false iff the NO_AUTO_START flag is set on the message.
  bool get_auto_start() const
  {
    return sd_bus_message_get_auto_start(m_message);
  }

  // Return true if the message is empty, i.e. when its signature is empty.
  bool is_empty() const
  {
    return sd_bus_message_is_empty(m_message);
  }

  // Returns true if the signature of the message message matches given signature.
  // Parameter signature may be NULL, this is treated the same as an empty string,
  // which is equivalent to calling is_empty().
  bool has_signature(char const* signature) const
  {
    return sd_bus_message_has_signature(m_message, signature);
  }

  char const* get_signature(bool complete = true) const
  {
    return sd_bus_message_get_signature(m_message, complete);
  }
};

class MessageRead : public MessageConst
{
  using MessageConst::MessageConst;

 public:
  MessageRead(sd_bus_message* message, sd_bus* bus) : MessageConst(message, bus) { }
  MessageRead& operator=(MessageRead&& message) { return static_cast<MessageRead&>(MessageConst::operator=(std::move(message))); }
  MessageRead& operator=(sd_bus_message* message) { this->MessageConst::operator=(message); return *this; }

  bool is_method_call(char const* interface, char const* member) const
  {
    return sd_bus_message_is_method_call(m_message, interface, member);
  }

  bool at_end(bool complete = true) const
  {
    int ret = sd_bus_message_at_end(m_message, complete);
    // Bug in program.
    ASSERT(ret >= 0);
    return ret;
  }
  void enter_container(char type, char const* contents) const
  {
    sd_bus_message_enter_container(m_message, type, contents);
  }
  void exit_container() const
  {
    sd_bus_message_exit_container(m_message);
  }

  MessageRead const& operator>>(uint8_t& n) const
  {
    int ret = sd_bus_message_read(m_message, "y", &n);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_message_read");
    return *this;
  }

  MessageRead const& operator>>(bool& b) const
  {
    int n;
    int ret = sd_bus_message_read(m_message, "b", &n);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_message_read");
    b = n;
    return *this;
  }

  MessageRead const& operator>>(int16_t& n) const
  {
    int ret = sd_bus_message_read(m_message, "n", &n);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_message_read");
    return *this;
  }

  MessageRead const& operator>>(uint16_t& n) const
  {
    int ret = sd_bus_message_read(m_message, "q", &n);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_message_read");
    return *this;
  }

  MessageRead const& operator>>(int32_t& n) const
  {
    int ret = sd_bus_message_read(m_message, "i", &n);
    // Do not try to read something that isn't there.
    // Call at_end(false) before trying to read from a container.
    ASSERT(ret != 0);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_message_read");
    return *this;
  }

  MessageRead const& operator>>(uint32_t& n) const
  {
    int ret = sd_bus_message_read(m_message, "u", &n);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_message_read");
    return *this;
  }

  MessageRead const& operator>>(int64_t& n) const
  {
    int ret = sd_bus_message_read(m_message, "x", &n);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_message_read");
    return *this;
  }

  MessageRead const& operator>>(uint64_t& n) const
  {
    int ret = sd_bus_message_read(m_message, "t", &n);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_message_read");
    return *this;
  }

  MessageRead const& operator>>(double& d) const
  {
    int ret = sd_bus_message_read(m_message, "d", &d);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_message_read");
    return *this;
  }

  MessageRead const& operator>>(std::string& s) const
  {
    char const* str;
    int ret = sd_bus_message_read(m_message, "s", &str);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_message_read");
    s = str;
    return *this;
  }

  template<typename CONTAINER>
  MessageRead const& operator>>(std::back_insert_iterator<CONTAINER> bi) const;

  bool peek_type(char& type, char const*& contents) const
  {
    int ret = sd_bus_message_peek_type(m_message, &type, &contents);
    // Bug in program.
    ASSERT(ret >= 0);
    return ret;
  }

  bool peek_type(char& type) const
  {
    int ret = sd_bus_message_peek_type(m_message, &type, nullptr);
    // Bug in program.
    ASSERT(ret >= 0);
    return ret;
  }

  operator sd_bus_message*() const { return m_message; }
};

template<typename T> char get_type();
template<> char get_type<uint8_t>();
template<> char get_type<bool>();
template<> char get_type<int16_t>();
template<> char get_type<uint16_t>();
template<> char get_type<int32_t>();
template<> char get_type<uint32_t>();
template<> char get_type<int64_t>();
template<> char get_type<uint64_t>();
template<> char get_type<double>();
template<> char get_type<std::string>();

template<typename CONTAINER>
MessageRead const& MessageRead::operator>>(std::back_insert_iterator<CONTAINER> bi) const
{
  char type;
  char const* contents;
  bool have_data = peek_type(type, contents);
  // Only arrays are supported at the moment.
  if (!have_data)
    THROW_FALERT("Received dbus message with unexpected end of content, expected array.");
  if (type != 'a')
    THROW_FALERT("Received dbus message with unsupported type [TYPE], expected array.", AIArgs("[TYPE]", type));
  enter_container(type, contents);
  if (dbus::get_type<typename CONTAINER::value_type>() != contents[0])
    THROW_FALERT("Dbus message with unexpected array contents [CONTENTS], expected [TYPE].", AIArgs("CONTENTS]", contents[0])("[TYPE]", dbus::get_type<typename CONTAINER::value_type>()));
  while (!at_end(false))
  {
    typename CONTAINER::value_type data;
    *this >> data;
    bi = data;
  }
  exit_container();
  return *this;
}

class Message : public MessageRead
{
 public:
  Message() = default;
  using MessageRead::MessageRead;

  void create_message(boost::intrusive_ptr<task::DBusConnection const> const& dbus_connection, Destination const& destination)
  {
    // Only call this after using the default constructor.
    ASSERT(m_message == nullptr);
    // This should be called from task::DBusMethodCall only after the DBusConnection is finished.
    ASSERT(dbus_connection->finished() && !dbus_connection->aborted());
    int ret = sd_bus_message_new_method_call(dbus_connection->get_bus(), &m_message,
        destination.service_name(), destination.object_path(), destination.interface_name(), destination.method_name());
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_message_new_method_call");
    m_bus = dbus_connection->get_bus();
  }

  // Append a std::vector or std::array (must be contiguous memory!)
  // value_type must be one of the types for which get_type is specialized (see above; i.e. do not use 'int').
  template<std::contiguous_iterator InputIt>
  Message& append(InputIt first, InputIt last)
  {
    using value_type = typename InputIt::value_type;
    char type = dbus::get_type<value_type>();
    int res;
    if constexpr (std::is_same_v<bool, value_type>)
    {
      std::vector<int> booleans;
      std::for_each(first, last, [&](bool boolean){ booleans.push_back(boolean); });
      res = sd_bus_message_append_array(m_message, type, &booleans[0], sizeof(int) * booleans.size());
    }
    else if constexpr (std::is_same_v<std::string, value_type>)
    {
      std::vector<char const*> strings;
      std::for_each(first, last, [&](std::string const& str){ strings.push_back(str.c_str()); });
      res = sd_bus_message_append_array(m_message, type, &strings[0], sizeof(char const*) * strings.size());
    }
    else
    {
      size_t number_of_elements = std::distance(first, last);
#ifdef CWDEBUG
      if (number_of_elements > 1)
      {
        auto real_last = last - 1;
        // Only use contiguous containers.
        ASSERT(number_of_elements - 1 == real_last - first);
      }
#endif
      res = sd_bus_message_append_array(m_message, type, &*first, sizeof(value_type) * number_of_elements);
    }
    if (res < 0)
      THROW_ALERTC(-res, "sd_bus_message_append_array");
    return *this;
  }

  template<typename T>
  Message& append(T basic_type)
  {
    char type = dbus::get_type<T>();
    int res;
    if constexpr (std::is_same_v<T, std::string>)
      res = sd_bus_message_append_basic(m_message, type, basic_type.c_str());
    else
      res = sd_bus_message_append_basic(m_message, type, reinterpret_cast<void*>(basic_type));
    if (res < 0)
      THROW_ALERTC(-res, "sd_bus_message_append_basic");
    return *this;
  }

  void reply_method_return(std::string const& result)
  {
    int ret = sd_bus_reply_method_return(m_message, "s", result.c_str());
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_reply_method_return");
  }
};

} // namespace dbus
