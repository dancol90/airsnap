#include "AvahiDiscoveryService.h"

#include <mutex>
#include <string>
#include <format>
#include <iostream>

#include <avahi-client/publish.h>
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/thread-watch.h>
#include <avahi-common/timeval.h>

class AvahiCallbacks
{
public:

  static void client_callback(AvahiClient *c, AvahiClientState state, void *userdata)
  {
    auto self = static_cast<AvahiDiscoveryService*>(userdata);

    switch (state)
    {
      case AVAHI_CLIENT_S_RUNNING:
        std::cout << std::format("[trace] Zeroconf: avahi client up and running.") << std::endl;
        avahi_threaded_poll_start(self->threaded_poll);
        break;
      case AVAHI_CLIENT_FAILURE:
        std::cout << std::format("[error] Zeroconf: avahi client failure [{}]", avahi_strerror(avahi_client_errno(c))) << std::endl;
        avahi_threaded_poll_stop(self->threaded_poll);
        break;
    }
  }

  static void discovery_callback(AvahiServiceBrowser *browser, AvahiIfIndex interface, AvahiProtocol protocol,
                                 AvahiBrowserEvent event, const char *name, const char *type, const char *domain,
                                 AvahiLookupResultFlags /*flags*/, void* userdata)
  {

    auto self = reinterpret_cast<AvahiDiscoveryService*>(userdata);
    assert(browser);

    switch (event)
    {
      case AVAHI_BROWSER_FAILURE:
        std::cout << std::format("[error] Zeroconf: browser failure [{}]", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(browser)))) << std::endl;
        avahi_threaded_poll_quit(self->threaded_poll);
        break;

      case AVAHI_BROWSER_NEW:
      {
        AvahiServiceResolver* resolver = avahi_service_resolver_new(
          self->client,
          interface, protocol,
          name, type, domain,
          AVAHI_PROTO_INET,
          static_cast<AvahiLookupFlags>(0),
          AvahiCallbacks::resolve_callback, self
        );

        //if (!resolver)
          std::cout << std::format("[error] Zeroconf: avahi resolver failure (avahi daemon problem) [{}][{}]", name, avahi_strerror(avahi_client_errno(self->client))) << std::endl;
        break;
      }
      case AVAHI_BROWSER_REMOVE:
      {
        std::cout << std::format("[trace] Zeroconf: service was removed [{}][{}][{}][{}]", name, type, domain, interface) << std::endl;

        std::lock_guard<std::mutex> lock(self->m_mutex);
        std::string key = std::format("{}|{}|{}", name, type, domain);
        
        auto it = self->m_services.find(key);
        if (it != self->m_services.end())
        {
          if (self->m_cb)
            self->m_cb(it->second, AvahiDiscoveryService::Event::Removed);

          self->m_services.erase(it);
        }
      
        break;
      }
      case AVAHI_BROWSER_ALL_FOR_NOW:
        std::cout << std::format("[trace] Zeroconf: cache exhausted") << std::endl;
        break;
      case AVAHI_BROWSER_CACHE_EXHAUSTED:
        std::cout << std::format("[trace] Zeroconf: all for now") << std::endl;
        break;
    }
  }

  static void resolve_callback(AvahiServiceResolver* resolver, AvahiIfIndex interface, AvahiProtocol protocol,
                              AvahiResolverEvent event, const char *name, const char *type, const char *domain,
                              const char *host_name, const AvahiAddress *address, uint16_t port, AvahiStringList *txt,
                              AvahiLookupResultFlags flags, void* userdata)
{

  auto self = reinterpret_cast<AvahiDiscoveryService*>(userdata);
  
  switch (event)
  {
    case AVAHI_RESOLVER_FAILURE:
      std::cout << std::format("[warning] Zeroconf: timed out resolving service [{}][{}][{}][{}]", name, type, domain, interface) << std::endl;
      break;
    case AVAHI_RESOLVER_FOUND:
    {
      std::cout << std::format("[warning] Zeroconf: found or update service [{}][{}][{}][{}]", name, type, domain, interface) << std::endl;

      std::lock_guard<std::mutex> lock(self->m_mutex);

      std::string key = std::format("{}|{}|{}", name, type, domain);
      auto it = self->m_services.try_emplace(key);
      auto & service = it.first->second;

      // Whether it existed already or not, update all the infos
      service.name = name;
      service.type = type;
      service.domain = domain;
      service.hostname = host_name;
      service.port = port;

      // Get resolved ip address
      char ip_str[AVAHI_ADDRESS_STR_MAX];
      avahi_address_snprint(ip_str, sizeof(ip_str), address);
      service.addresses.insert(ip_str);

      // Parse the TXT record as a dictionary
      service.description.clear();
      auto list = txt;
      while (list)
      {
        char * key;
        char * value;

        avahi_string_list_get_pair(list, &key, &value, nullptr);
        service.description[key] = value;

        avahi_free(key);
        avahi_free(value);

        list = avahi_string_list_get_next(list);
      }

      if (self->m_cb)
        self->m_cb(service, it.second ? AvahiDiscoveryService::Event::Added : AvahiDiscoveryService::Event::Updated);

      break;
    }
  }
}
};

AvahiDiscoveryService::AvahiDiscoveryService(const std::string & service) :
  threaded_poll(nullptr),
  client(nullptr),
  service_browser(nullptr)
{
  /* Allocate main loop object */
  threaded_poll = avahi_threaded_poll_new();

  if (!threaded_poll)
  {
    std::cout << std::format("[error] Zeroconf: failed to create an avahi threaded  poll.") << std::endl;
    return;
  }

  /* Allocate a new client */
  int error;
  client = avahi_client_new(
    avahi_threaded_poll_get(threaded_poll),
    static_cast<AvahiClientFlags>(0),
    AvahiCallbacks::client_callback, this,
    &error
  );

  if (!client)
  {
    std::cout << std::format("[error] Zeroconf: failed to create an avahi client: {}", avahi_strerror(error)) << std::endl;
    return;
  }

  /* Create the service browser */  
  avahi_threaded_poll_lock(threaded_poll);
  service_browser = avahi_service_browser_new(
    client, 
    AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, 
    service.c_str(), nullptr,
    static_cast<AvahiLookupFlags>(0), 
    AvahiCallbacks::discovery_callback, this
  );
  avahi_threaded_poll_unlock(threaded_poll);

  if (!service_browser)
  {
    std::cout << std::format("[error] Zeroconf: failed to create an avahi service browser: {}", avahi_strerror(avahi_client_errno(client))) << std::endl;
    return;
  }

}

AvahiDiscoveryService::~AvahiDiscoveryService()
{
  if (threaded_poll)
    avahi_threaded_poll_stop(threaded_poll);

  if (service_browser)
    avahi_service_browser_free(service_browser);
  service_browser = nullptr;

  if (client)
    avahi_client_free(client);
  client = nullptr;
  
  if (threaded_poll)
    avahi_threaded_poll_free(threaded_poll);
  threaded_poll = nullptr;
}

std::vector<DiscoveredService> AvahiDiscoveryService::getDiscoveredServices() const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  std::vector<DiscoveredService> svcs;
  for (auto & svc : m_services)
    svcs.push_back(svc.second);
  return svcs;
}