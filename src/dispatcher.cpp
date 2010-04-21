#include "cow/libcow_def.hpp"
#include "cow/dispatcher.hpp"

#include <iostream>

dispatcher::dispatcher()
    : work(io_service),
      thread(boost::bind(&boost::asio::io_service::run, &io_service))
{

}

dispatcher::~dispatcher()
{
#ifdef DEBUG
    std::cerr << "dispatcher: waiting for running jobs to complete..."
              << std::endl;
#endif
    io_service.stop();
    thread.join();
}
