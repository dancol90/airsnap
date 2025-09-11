#include "AvahiDiscoveryService.h"

#include <iostream>
#include <thread>
#include <format>

// Linux only
#include <signal.h>

#include <boost/process.hpp>

using namespace std::chrono_literals;
namespace bp = boost::process;

void signal_handler(int signal)
{
}

int main()
{
  AvahiDiscoveryService zeroconf("_raop._tcp");

  zeroconf.setCallback([](const DiscoveredService service, AvahiDiscoveryService::Event event)
  {
    switch (event)
    {
    case AvahiDiscoveryService::Event::Added:
      std::cout << std::format("Added service {} {}", service.name, *service.addresses.begin()) << std::endl;
      break;
    case AvahiDiscoveryService::Event::Updated:
      std::cout << "Update service " << service.name << std::endl;
      break;
    case AvahiDiscoveryService::Event::Removed:
      std::cout << "Removed service " << service.name << std::endl;
      break;
    }
  });

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);


  // bp::ipstream is; //reading pipe-stream
  // bp::child c(bp::search_path("nm"), file, bp::std_out > is);

  // std::vector<std::string> data;
  // std::string line;

  // while (c.running() && std::getline(is, line) && !line.empty())
  //     data.push_back(line);

  // c.wait();


  std::this_thread::sleep_for(30s);
  return 0;
}