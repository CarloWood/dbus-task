#include "sys.h"
#include "Message.h"

namespace dbus {

template<>
char get_type<uint8_t>()
{
  return 'y';
}

template<>
char get_type<bool>()
{
  return 'b';
}

template<>
char get_type<int16_t>()
{
  return 'n';
}

template<>
char get_type<uint16_t>()
{
  return 'q';
}

template<>
char get_type<int32_t>()
{
  return 'i';
}

template<>
char get_type<uint32_t>()
{
  return 'u';
}

template<>
char get_type<int64_t>()
{
  return 'x';
}

template<>
char get_type<uint64_t>()
{
  return 't';
}

template<>
char get_type<double>()
{
  return 'd';
}

template<>
char get_type<std::string>()
{
  return 's';
}

} // namespace dbus
