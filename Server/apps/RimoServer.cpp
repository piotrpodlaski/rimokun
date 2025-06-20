#include "RimoServer.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <numbers>
#include <thread>
#include <print>

#include "logger.hpp"

using namespace utl;

using namespace std::chrono_literals;

std::atomic tclOpen{ELEDState::Off};
std::atomic tclClosed{ELEDState::Off};
std::atomic tclProx{ELEDState::ErrorBlinking};
std::atomic tclValveOpen{ELEDState::Off};
std::atomic tclValveClosed{ELEDState::Off};

std::atomic tcrOpen{ELEDState::Off};
std::atomic tcrClosed{ELEDState::Off};
std::atomic tcrProx{ELEDState::ErrorBlinking};
std::atomic tcrValveOpen{ELEDState::Off};
std::atomic tcrValveClosed{ELEDState::Off};

RobotStatus prepareFakeStatus() {
  static double t = 0;
  constexpr double dt = 0.05;
  RobotStatus status;
  const std::vector motors = {EMotor::XLeft,  EMotor::XRight, EMotor::YLeft,
                              EMotor::YRight, EMotor::ZLeft,  EMotor::ZRight};
  const std::map<EMotor, double> amplitudes = {
      {EMotor::XLeft, 500},  {EMotor::YLeft, 300}, {EMotor::XRight, 500},
      {EMotor::YRight, 300}, {EMotor::ZLeft, 100}, {EMotor::ZRight, 100}};

  const std::map<EMotor, double> offsets = {
      {EMotor::XLeft, 1500},  {EMotor::YLeft, -500}, {EMotor::XRight, 3000},
      {EMotor::YRight, -500}, {EMotor::ZLeft, 100},  {EMotor::ZRight, 100}};

  const std::map<EMotor, double> omegas = {
      {EMotor::XLeft, 2},  {EMotor::YLeft, 2}, {EMotor::XRight, 1},
      {EMotor::YRight, 3}, {EMotor::ZLeft, 1}, {EMotor::ZRight, 1}};

  const std::map<EMotor, double> phases = {
      {EMotor::XLeft, 0},  {EMotor::YLeft, std::numbers::pi / 2},
      {EMotor::XRight, 0}, {EMotor::YRight, 0},
      {EMotor::ZLeft, 0},  {EMotor::ZRight, 0}};
  const std::vector<std::pair<EMotor, EMotorStatusFlags>> vFlags = {
      {EMotor::XLeft, EMotorStatusFlags::BrakeApplied},
      {EMotor::XLeft, EMotorStatusFlags::Enabled},
      {EMotor::YLeft, EMotorStatusFlags::BrakeApplied},
      {EMotor::YLeft, EMotorStatusFlags::Enabled},
      {EMotor::XRight, EMotorStatusFlags::BrakeApplied},
      {EMotor::XRight, EMotorStatusFlags::Enabled},
      {EMotor::YRight, EMotorStatusFlags::BrakeApplied},
      {EMotor::YRight, EMotorStatusFlags::Enabled},
      {EMotor::ZLeft, EMotorStatusFlags::BrakeApplied},
      {EMotor::ZLeft, EMotorStatusFlags::Enabled},
      {EMotor::ZRight, EMotorStatusFlags::BrakeApplied},
      {EMotor::ZRight, EMotorStatusFlags::Enabled}};

  auto [fst, snd] = vFlags.at(static_cast<int>(3 * t) % vFlags.size());
  for (const auto& motor : motors) {
    const auto A = amplitudes.at(motor);
    const auto omega = omegas.at(motor);
    const auto phase = phases.at(motor);
    const auto offset = offsets.at(motor);
    status.motors[motor] = {
        .currentPosition = A * std::sin(omega * t + phase) + offset,
        .targetPosition = 0,
        .speed = A * omega * std::cos(omega * t + phase),
        .torque = static_cast<int>(
            10 * std::fabs(omega * omega * std::sin(omega * t + phase)))};
    if (motor == fst) status.motors[motor].flags[snd] = ELEDState::On;
  }
  t += dt;

  status.toolChangers[EArm::Left] = {
      .flags = {{EToolChangerStatusFlags::ProxSen, tclProx},
                {EToolChangerStatusFlags::ClosedSen, tclClosed},
                {EToolChangerStatusFlags::OpenSen, tclOpen},
                {EToolChangerStatusFlags::OpenValve, tclValveOpen},
                {EToolChangerStatusFlags::ClosedValve, tclValveClosed}}};
  status.toolChangers[EArm::Right] = {
      .flags = {{EToolChangerStatusFlags::ProxSen, tcrProx},
                {EToolChangerStatusFlags::ClosedSen, tcrClosed},
                {EToolChangerStatusFlags::OpenSen, tcrOpen},
                {EToolChangerStatusFlags::OpenValve, tcrValveOpen},
                {EToolChangerStatusFlags::ClosedValve, tcrValveClosed}}};
  return status;
}

[[noreturn]] void handleCommands(RimoServer<RobotStatus>& srv) {
  while (true) {
    if (auto command = srv.receiveCommand()) {
      SPDLOG_INFO("Received command:\n{}\n", YAML::Dump(*command));
      YAML::Node node;
      node["status"]="OK";
      node["message"]="";
      srv.sendResponse(node);
      if ((*command)["type"].as<std::string>()=="toolChanger") {
        auto position = (*command)["position"].as<EArm>();
        auto action = (*command)["action"].as<std::string>();
        if (position == EArm::Left) {
          if (action == "close") {
            tclValveOpen=ELEDState::Off;
            tclValveClosed=ELEDState::On;
            std::this_thread::sleep_for(0.5s);
            tclClosed=ELEDState::On;
            tclOpen=ELEDState::Off;
          }
          else if (action == "open") {
            tclValveOpen=ELEDState::On;
            tclValveClosed=ELEDState::Off;
            std::this_thread::sleep_for(0.5s);
            tclClosed=ELEDState::Off;
            tclOpen=ELEDState::On;
          }
        }
        else if (position == EArm::Right) {
          if (action == "close") {
            tcrValveOpen=ELEDState::Off;
            tcrValveClosed=ELEDState::On;
            std::this_thread::sleep_for(0.5s);
            tcrClosed=ELEDState::On;
            tcrOpen=ELEDState::Off;
          }
          else if (action == "open") {
            tcrValveOpen=ELEDState::On;
            tcrValveClosed=ELEDState::Off;
            std::this_thread::sleep_for(0.5s);
            tcrClosed=ELEDState::Off;
            tcrOpen=ELEDState::On;
          }
        }
      }
    }
  }
}

[[noreturn]] int main(int argc, char** argv) {
  configure_logger();
  std::cout << "Hello World!\n";
  auto srv = RimoServer<RobotStatus>();
  std::thread commandThread{&handleCommands, std::ref(srv)};

  while (true) {
    srv.publish(prepareFakeStatus());
    std::this_thread::sleep_for(100ms);
  }
  commandThread.join();
}