#include "sys.h"
#include "utils/AIAlert.h"
#include "utils/debug_ostream_operators.h"
#include <systemd/sd-bus.h>
#include <string>
#include <vector>
#include <iostream>
#include "debug.h"

static int method_concatenate(sd_bus_message* m, void* userdata, sd_bus_error* ret_error)
{
  // Deserialize the collection of numbers from the message.
  std::vector<int> numbers;

  int ret = sd_bus_message_enter_container(m, 'a', "i");
  if (ret < 0)
    THROW_ALERTC(-ret, "sd_bus_message_enter_container");

  for (;;)
  {
    int n;
    ret = sd_bus_message_read(m, "i", &n);
    if (ret < 0)
      THROW_ALERTC(-ret, "sd_bus_message_read");
    if (ret == 0)
      break;
    numbers.push_back(n);
  }

  ret = sd_bus_message_exit_container(m);
  if (ret < 0)
    THROW_ALERTC(-ret, "sd_bus_message_exit_container");

  if (numbers.empty())
  {
    sd_bus_error_set(ret_error, "org.sdbuscpp.Concatenator.Error.NoNumbers", "No numbers provided");
    return 0;
  }

  char const* separator;
  ret = sd_bus_message_read(m, "s", &separator);
  if (ret < 0)
    THROW_ALERTC(-ret, "sd_bus_message_read");

  std::string result;
  for (auto number : numbers) { result += (result.empty() ? std::string() : separator) + std::to_string(number); }

  ret = sd_bus_reply_method_return(m, "s", result.c_str());
  if (ret < 0)
    THROW_ALERTC(-ret, "sd_bus_reply_method_return");

  // Emit 'concatenated' signal
  char const* interfaceName = "org.sdbuscpp.Concatenator";
  char const* objectPath    = "/org/sdbuscpp/concatenator";

  sd_bus* bus = static_cast<sd_bus*>(userdata);
  ret = sd_bus_emit_signal(bus, objectPath, interfaceName, "concatenated", "s", result.c_str());
  if (ret < 0)
    THROW_ALERTC(-ret, "sd_bus_emit_signal");

  return 0;
}

/* The vtable of our little object, implements the net.poettering.Calculator interface */
static const sd_bus_vtable concatenator_vtable[] = {
  SD_BUS_VTABLE_START(0),
  SD_BUS_METHOD("concatenate", "ais", "s", method_concatenate, SD_BUS_VTABLE_UNPRIVILEGED),
  SD_BUS_VTABLE_END
};

int main()
{
  Debug(debug::init());
  Dout(dc::notice, "Entering main()");

  // Create D-Bus connection to the system bus and requests name on it.
  char const* serviceName   = "org.sdbuscpp.concatenator";
  char const* objectPath    = "/org/sdbuscpp/concatenator";
  char const* interfaceName = "org.sdbuscpp.Concatenator";

  sd_bus_slot* slot = nullptr;
  sd_bus* bus       = nullptr;

  try
  {
    // Connect to the user bus.
    int r = sd_bus_open_user(&bus);
    if (r < 0)
    {
      std::cerr << "Failed to connect to system bus: " << std::strerror(-r) << std::endl;
      goto finish;
    }

    // Install the object.
    r = sd_bus_add_object_vtable(bus, &slot, objectPath, interfaceName, concatenator_vtable, bus);
    if (r < 0)
    {
      std::cerr << "Failed to issue method call: " << std::strerror(-r) << std::endl;
      goto finish;
    }

    r = sd_bus_request_name(bus, serviceName, 0);
    if (r < 0)
    {
      std::cerr << "Failed to acquire service name: " << std::strerror(-r) << std::endl;
      goto finish;
    }

    for (;;)
    {
      // Process requests.
      r = sd_bus_process(bus, nullptr);
      if (r < 0)
      {
        std::cerr << "Failed to process bus: " << std::strerror(-r) << std::endl;
        goto finish;
      }
      if (r > 0)  // We processed a request, try to process another one, right-away.
        continue;

      // Wait for the next request to process.
      r = sd_bus_wait(bus, (uint64_t)-1);
      if (r < 0)
      {
        std::cerr << "Failed to wait on bus: " << std::strerror(-r) << std::endl;
        goto finish;
      }
    }

  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error);
  }

finish:
  sd_bus_slot_unref(slot);
  sd_bus_unref(bus);

  Dout(dc::notice, "Leaving main()");
}
