//
// Created by piotrek on 6/10/25.
//

#include "RimoServer.hpp"

#include <yaml-cpp/yaml.h>

#include <chrono>

#include "CommonDefinitions.hpp"
#include "RimoClient.hpp"
#include "logger.hpp"

using namespace std::chrono_literals;

namespace utl {

 RimoServer::RimoServer() {
   _socket = zmq::socket_t(_context, zmq::socket_type::pub);
   _socket.bind("ipc:///tmp/rimo_server");
 }

void RimoServer::publish(const RobotStatus& robot) {
   auto node = YAML::convert<RobotStatus>::encode(robot);
   auto yaml_str = YAML::Dump(node);
   zmq::message_t msg(yaml_str);
   _socket.send(msg, zmq::send_flags::none);
   SPDLOG_DEBUG("Published new RobotStatus");
   SPDLOG_TRACE("RobotStatus: \n {}", yaml_str);
 }



// void RimoServer::publisherThread() {
//   double t=0;
//   while (true) {
//     RobotStatus status = {
//       {{EMotor::XLeft,
//         {.currentPosition = 200+100.*std::sin(t),
//          .targetPosition = 0.,
//          .speed = 100.*std::cos(t),
//           .torque = static_cast<int>(std::abs(100.*std::sin(t))),
//          .flags = {{EMotorStatusFlags::BrakeApplied, ELEDState::On}}}},
//        {EMotor::XRight,
//         {.currentPosition = 2.,
//          .targetPosition = 3.,
//          .speed = 11.,
//           .torque = 0,
//          .flags = {}}},
// {EMotor::YLeft,
// {.currentPosition = 2.,
//  .targetPosition = 3.,
//  .speed = 11.,
//  .flags = {}}}
//       }};
//     t+=0.01;
//     auto node = YAML::convert<RobotStatus>::encode(status);
//     std::print("Publishing!\n");
//     auto yaml_str = YAML::Dump(node);
//     std::print("{}\n", yaml_str);
//     zmq::message_t msg(yaml_str.data(), yaml_str.size());
//     publisher.send(msg, zmq::send_flags::none);
//     std::this_thread::sleep_for(50ms  );
//   }
// }


}  // namespace utl