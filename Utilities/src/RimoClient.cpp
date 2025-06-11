//
// Created by piotrek on 6/10/25.
//

#include "RimoClient.hpp"

#include <print>

#include "logger.hpp"

using namespace std::chrono_literals;

namespace utl {
void RimoClient::spawn() {
  if (m_running) {
    SPDLOG_WARN("Thread is already running!");
    return;
  }
  SPDLOG_INFO("Spawning subscriber thread");
  if (_subscriberThread.joinable()) {
    _subscriberThread.join();
  }
  m_running = true;
  _subscriberThread = std::thread(&RimoClient::subscriberThread, this);
}

void RimoClient::subscriberThread() {
  std::this_thread::sleep_for(500ms);
  zmq::socket_t subscriber(_context, zmq::socket_type::sub);
  subscriber.connect("ipc:///tmp/rimo_server");
  subscriber.set(zmq::sockopt::subscribe, "");
  while (m_running) {
    zmq::message_t message;
    if (!subscriber.recv(message)) continue;
    auto msg_str = message.to_string();
    SPDLOG_DEBUG("Received message from publisher!");
    SPDLOG_TRACE("message:\n{}", msg_str);
    auto [motors] = YAML::Load(msg_str).as<RobotStatus>();
    if (!_motorStats) continue;
    for (const auto& [m, s] : motors) {
      if (_motorStats->contains(m)) {
        _motorStats->at(m)->configure(s);
      }
    }
  }
}

void RimoClient::stop() {
  m_running = false;
  _subscriberThread.join();
}

RimoClient::~RimoClient() { stop(); }

bool RimoClient::isRunning() const { return m_running; }
void RimoClient::setMotorStatsPtr(MotorStatsMap_t* stat) { _motorStats = stat; }

}  // namespace utl