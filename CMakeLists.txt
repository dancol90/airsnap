cmake_minimum_required(VERSION 3.25)
project(AirSnap)

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(Avahi REQUIRED)
find_package(Boost REQUIRED CONFIG)

add_executable(airsnap
  main.cpp
  DiscoveredService.h
  AvahiDiscoveryService.h
  AvahiDiscoveryService.cpp
  ClientProcess.h
)

target_link_libraries(airsnap 
  Avahi::client
  Boost::headers
)

target_compile_definitions(airsnap PUBLIC BOOST_PROCESS_USE_STD_FS)