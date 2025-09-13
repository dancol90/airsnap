#pragma once

#ifndef DISCOVERED_SERVICE
#define DISCOVERED_SERVICE

#include <map>
#include <set>
#include <string>
#include <compare>

struct DiscoveredService
{
  std::string name;
  std::string type;
  std::string domain;
  std::map<std::string, std::string> description;

  std::string hostname;
  std::set<std::string> addresses;
  int port;

  inline std::strong_ordering operator<=>(const DiscoveredService& other) const;
  inline bool operator==(const DiscoveredService& other) const;
};

std::strong_ordering DiscoveredService::operator<=>(const DiscoveredService& other) const
{
  auto c = name <=> other.name;
  if (c != 0)
    return c;

  c = type <=> other.type;
  if (c != 0)
    return c;

  return domain <=> other.domain;
}

bool DiscoveredService::operator==(const DiscoveredService& other) const
{
  return (*this <=> other) == std::strong_ordering::equal;
}


#endif // DISCOVERED_SERVICE