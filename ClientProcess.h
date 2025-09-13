#include "DiscoveredService.h"

#include <iostream>

#include <boost/process/v2.hpp>
#include <boost/asio.hpp>

namespace bp = boost::process::v2;

struct ClientProcess
{
  ClientProcess(boost::asio::io_context & ctx, DiscoveredService service) :
    m_active(true),
    m_service(std::move(service)),
    m_ctx(ctx),
    m_process(std::move(create_process()))
  {
    m_process.async_wait(std::bind_front(&ClientProcess::on_exit, this));
  }

  ~ClientProcess()
  {
    std::cout << "Terminating process for " << m_service.name << std::endl;
    
    boost::system::error_code ec;
    if (m_process.running(ec))
    {
        // SIGTERM and blocking wait for the process to exit
        m_process.request_exit();
        m_process.wait();
    }
    else
        std::cout << "ec: " << ec.message() << std::endl;
  }

  inline const DiscoveredService & service() const { return m_service; }
  
private:

  bp::process create_process()
  {
    std::cout << "Starting process for " << m_service.name << std::endl;
    auto p = bp::process(
      m_ctx,
      bp::environment::find_executable("sleep"),
      // Arguments
      { "15" },
      // No stdin, stdout&stderr to parent
      bp::process_stdio{nullptr, {}, {}}
    );

    std::cout << std::format("Started with PID {}", p.id()) << std::endl;
    return p;
  }

  void on_exit(boost::system::error_code ec, int exit_code)
  {
    if (ec.failed())
    {
      std::cout << std::format("[ERROR] Can't wait for process completion: {}", ec.message()) << std::endl;
      m_active = false;
      return;
    }

    std::cout << std::format("Process {} exited with code {} [{}]", m_process.id(), exit_code, ec.message()) << std::endl;
    if (m_active)
        m_process = create_process();
  }

  bool m_active;
  DiscoveredService m_service;
  boost::asio::io_context & m_ctx;
  bp::process m_process;
};
