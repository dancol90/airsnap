#include "AvahiDiscoveryService.h"
#include "ClientProcess.h"

#include <iostream>
#include <thread>
#include <format>
#include <forward_list>

#include <boost/asio.hpp>

using namespace std::chrono_literals;

int main()
{
  boost::asio::io_context ctx;
  std::forward_list<ClientProcess> clients;
  AvahiDiscoveryService zeroconf("_raop._tcp");

  zeroconf.setCallback([&](const DiscoveredService service, AvahiDiscoveryService::Event event)
  {
    switch (event)
    {
    case AvahiDiscoveryService::Event::Added:
      std::cout << std::format(">>>>>>>>>>> Found new service {} {}", service.name, *service.addresses.begin()) << std::endl;
      clients.remove_if([&](auto & i) { return i.service() == service; });
      clients.emplace_front(ctx, service);
      break;

    case AvahiDiscoveryService::Event::Updated:
      std::cout << ">>>>>>>>>>> Updated service " << service.name << std::endl;
      break;

    case AvahiDiscoveryService::Event::Removed:
      std::cout << ">>>>>>>>>>> Removed service " << service.name << std::endl;
      clients.remove_if([&](auto & i) { return i.service() == service; });
      break;

    }
  });

  // boost::asio::steady_timer tm(ctx, 20s);
  // tm.async_wait([&](boost::system::error_code ec)
  // {
  //   if (ec.failed())
  //   {
  //     std::cout << std::format("[ERROR] Can't wait for process completion: {}", ec.message()) << std::endl;
  //     return;
  //   }

  //   std::cout << std::format("Timer elapsed [{}]", ec.message()) << std::endl;
  // });

  // Setup signal handling to keep running until Ctrl-C
  boost::asio::signal_set signals(ctx, SIGINT, SIGTERM);
  signals.async_wait([](boost::system::error_code ec, int ret) {
    std::cout << std::format("Requested signal {}", ret) << std::endl;
  });

  // Run the IO context until all asio operations are completed
  ctx.run();

  std::cout << std::format("Exiting") << std::endl;
  return 0;
}