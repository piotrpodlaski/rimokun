//
// Created by piotrek on 6/10/25.
//

#pragma once

#include <thread>

#include "zmq.hpp"
#include "VMotorStats.hpp"

namespace utl {

class RimoClient {
 public:
  RimoClient() = default;
  ~RimoClient();
  void spawn();
  void stop();
  bool isRunning() const;
  void setMotorStatsPtr(MotorStatsMap_t* stat);

 private:
  bool m_running = false;
  void subscriberThread();
  zmq::context_t _context;
  std::thread _subscriberThread;
  MotorStatsMap_t* _motorStats{nullptr};
};

}  // namespace utl
