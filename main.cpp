#include "AvahiDiscoveryService.h"

#include <iostream>
#include <thread>
#include <format>

#include <csignal>

#include <boost/process/v2.hpp>
#include <boost/asio.hpp>

using namespace std::chrono_literals;
namespace bp = boost::process::v2;

struct ClientProcess
{
  ClientProcess(boost::asio::io_context & ctx, DiscoveredService service) :
    service(std::move(service)),
    process(std::move(create_process(ctx)))
  {
    process.async_wait(std::bind_front(&ClientProcess::on_exit, this));
  }

private:

  bp::process create_process(boost::asio::io_context & ctx)
  {
    return bp::process(
      ctx,
      //bp::environment::find_executable("sleep"),
      "/home/daniele/snapcast/bin/snapserver",
      // Arguments
      { "--config=/home/daniele/snapcast/server/etc/snapserver.conf" },
      // No stdin, stdout&stderr to parent
      bp::process_stdio{nullptr, {}, {}}
    );
  }

  void on_exit(boost::system::error_code ec, int exit_code)
  {
    if (ec.failed())
    {
      std::cout << std::format("[ERROR] Can't wait for process completion: {}", ec.message()) << std::endl;
      return;
    }

    std::cout << std::format("Process {} exited with code {} [{}]", process.id(), exit_code, ec.message()) << std::endl;
  }

  DiscoveredService service;
  bp::process process;
};

int main()
{
  boost::asio::io_context ctx;

  AvahiDiscoveryService zeroconf("_raop._tcp");

  zeroconf.setCallback([&](const DiscoveredService service, AvahiDiscoveryService::Event event)
  {
    switch (event)
    {
    case AvahiDiscoveryService::Event::Added:
      std::cout << std::format("Added service {} {}", service.name, *service.addresses.begin()) << std::endl;
      //boost::asio::post(ctx, []() {});
      break;
    case AvahiDiscoveryService::Event::Updated:
      std::cout << "Update service " << service.name << std::endl;
      break;
    case AvahiDiscoveryService::Event::Removed:
      std::cout << "Removed service " << service.name << std::endl;
      break;
    }
  });

  // boost::asio::steady_timer tm(ctx, 5s);
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