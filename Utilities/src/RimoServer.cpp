//
// Created by piotrek on 6/10/25.
//

#include "RimoServer.hpp"

#include <yaml-cpp/yaml.h>

#include <chrono>
#include <print>
#include <thread>

#include "CommonDefinitions.hpp"

namespace utl {
void RimoServer::publisherThread() {
  zmq::socket_t publisher(_context, zmq::socket_type::pub);
  publisher.bind("ipc:///tmp/rimo_server");
  RobotStatus status = {
      {{EMotor::XLeft,
        {.currentPosition = 1.,
         .targetPosition = 0.,
         .speed = 10.,
         .flags = {{EMotorStatusFlags::BrakeApplied, ELEDState::On}}}},
       {EMotor::XRight,
        {.currentPosition = 2.,
         .targetPosition = 3.,
         .speed = 11.,
         .flags = {}}},
{EMotor::YLeft,
{.currentPosition = 2.,
 .targetPosition = 3.,
 .speed = 11.,
 .flags = {}}}
      }};
  while (true) {
    auto node = YAML::convert<RobotStatus>::encode(status);
    std::print("Publishing!\n");
    auto yaml_str = YAML::Dump(node);
    std::print("{}\n", yaml_str);
    zmq::message_t msg(yaml_str.data(), yaml_str.size());
    publisher.send(msg, zmq::send_flags::none);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void RimoServer::spawn() {
  _publisherThread = std::thread(&RimoServer::publisherThread, this);
}

}  // namespace utl