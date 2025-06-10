//
// Created by piotrek on 6/10/25.
//

#include "RimoServer.hpp"

#include <yaml-cpp/yaml.h>

#include <chrono>
#include <print>
#include <thread>

#include "CommonDefinitions.hpp"

using namespace std::chrono_literals;

namespace utl {
void RimoServer::publisherThread() {
  zmq::socket_t publisher(_context, zmq::socket_type::pub);
  publisher.bind("ipc:///tmp/rimo_server");
  double t=0;
  while (true) {
    RobotStatus status = {
      {{EMotor::XLeft,
        {.currentPosition = 200+100.*std::sin(t),
         .targetPosition = 0.,
         .speed = 100.*std::cos(t),
          .torque = static_cast<int>(std::abs(100.*std::sin(t))),
         .flags = {{EMotorStatusFlags::BrakeApplied, ELEDState::On}}}},
       {EMotor::XRight,
        {.currentPosition = 2.,
         .targetPosition = 3.,
         .speed = 11.,
          .torque = 0,
         .flags = {}}},
{EMotor::YLeft,
{.currentPosition = 2.,
 .targetPosition = 3.,
 .speed = 11.,
 .flags = {}}}
      }};
    t+=0.01;
    auto node = YAML::convert<RobotStatus>::encode(status);
    std::print("Publishing!\n");
    auto yaml_str = YAML::Dump(node);
    std::print("{}\n", yaml_str);
    zmq::message_t msg(yaml_str.data(), yaml_str.size());
    publisher.send(msg, zmq::send_flags::none);
    std::this_thread::sleep_for(50ms  );
  }
}

void RimoServer::spawn() {
  _publisherThread = std::thread(&RimoServer::publisherThread, this);
}

}  // namespace utl