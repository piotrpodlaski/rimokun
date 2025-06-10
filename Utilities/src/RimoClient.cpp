//
// Created by piotrek on 6/10/25.
//

#include "RimoClient.h"
#include <print>

namespace utl {
void RimoClient::spawn() {
  if (m_running) {
    std::print("Thread already running!\n");
    return;
  }
  std::print("spawning thread!!\n");
  if (_subscriberThread.joinable()) {
    _subscriberThread.join();
  }
  m_running = true;
  _subscriberThread=std::thread(&RimoClient::subscriberThread, this);

}

void RimoClient::subscriberThread() {
  zmq::socket_t subscriber(_context, zmq::socket_type::sub);
  subscriber.connect("ipc:///tmp/rimo_server");
  subscriber.set(zmq::sockopt::subscribe, "");
  while (m_running) {
    zmq::message_t message;
    auto result = subscriber.recv(message);
    std::print("received:\n\t result: {} \n\tmessage {}\n",*result,message.to_string());

  }
}

void RimoClient::stop() {
  m_running = false;
  _subscriberThread.join();
}

 RimoClient::~RimoClient() {
  stop();
}

bool RimoClient::isRunning() const {
  return m_running;
}



} // utl