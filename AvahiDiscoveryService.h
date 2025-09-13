#pragma once

#ifndef AVAHI_DISCOVERY_SERVICE
#define AVAHI_DISCOVERY_SERVICE

#include <map>
#include <set>
#include <string>
#include <mutex>
#include <vector>
#include <functional>

#include "DiscoveredService.h"

struct AvahiThreadedPoll;
struct AvahiClient;
struct AvahiServiceBrowser;

class AvahiDiscoveryService
{
public:
  enum class Event
  {
    Added,
    Updated,
    Removed
  };

  using DiscoveryCallback = std::function<void(const DiscoveredService &, Event)>;

  AvahiDiscoveryService(const std::string & service);
  ~AvahiDiscoveryService();

  std::vector<DiscoveredService> getDiscoveredServices() const;

  void setCallback(DiscoveryCallback cb) { m_cb = cb; }
  
private:
  friend class AvahiCallbacks;
  
  AvahiThreadedPoll *threaded_poll;
  AvahiClient *client;
  AvahiServiceBrowser *service_browser;

  DiscoveryCallback m_cb;

  std::vector<DiscoveredService> m_services;
  mutable std::mutex m_mutex;
};

#endif // AVAHI_DISCOVERY_SERVICE