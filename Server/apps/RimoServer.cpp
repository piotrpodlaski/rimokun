#include "RimoServer.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <numbers>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

#include "Logger.hpp"
#include "Config.hpp"
#include "argparse/argparse.hpp"

using namespace utl;
using namespace std::chrono_literals;
namespace fs = std::filesystem;

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

namespace {
void applyToolChangerAction(EArm position, std::string_view action) {
  auto perform = [](auto& valveOpen, auto& valveClosed, auto& closedState,
                    auto& openState, bool shouldClose) {
    if (shouldClose) {
      valveOpen = ELEDState::Off;
      valveClosed = ELEDState::On;
      std::this_thread::sleep_for(0.1s);
      closedState = ELEDState::On;
      openState = ELEDState::Off;
    } else {
      valveOpen = ELEDState::On;
      valveClosed = ELEDState::Off;
      std::this_thread::sleep_for(0.1s);
      closedState = ELEDState::Off;
      openState = ELEDState::On;
    }
  };

  const bool shouldClose = (action == "close");
  if (position == EArm::Left) {
    perform(tclValveOpen, tclValveClosed, tclClosed, tclOpen, shouldClose);
  } else {
    perform(tcrValveOpen, tcrValveClosed, tcrClosed, tcrOpen, shouldClose);
  }
}

std::string validateAction(const YAML::Node& command) {
  const auto actionNode = command["action"];
  if (!actionNode || !actionNode.IsScalar()) {
    throw std::runtime_error("Missing or invalid 'action' entry");
  }
  const auto action = actionNode.as<std::string>();
  if (action != "open" && action != "close") {
    throw std::runtime_error("Unsupported tool changer action: " + action);
  }
  return action;
}

EArm validatePosition(const YAML::Node& command) {
  const auto positionNode = command["position"];
  if (!positionNode || !positionNode.IsScalar()) {
    throw std::runtime_error("Missing or invalid 'position' entry");
  }
  try {
    return positionNode.as<EArm>();
  } catch (const YAML::Exception& ex) {
    throw std::runtime_error(
        std::string("Failed to parse 'position' entry: ") + ex.what());
  }
}
}  // namespace

[[noreturn]] void handleCommands(RimoServer<RobotStatus>& srv) {
  while (true) {
    if (auto command = srv.receiveCommand()) {
      SPDLOG_INFO("Received command:\n{}", YAML::Dump(*command));
      YAML::Node response;
      response["status"] = "OK";
      response["message"] = "";
      try {
        if (!command->IsMap()) {
          throw std::runtime_error("Command must be a map");
        }
        const auto typeNode = (*command)["type"];
        if (!typeNode || !typeNode.IsScalar()) {
          throw std::runtime_error("Command lacks a valid 'type' entry");
        }
        const auto type = typeNode.as<std::string>();
        if (type == "toolChanger") {
          const auto position = validatePosition(*command);
          const auto action = validateAction(*command);
          applyToolChangerAction(position, action);
        } else {
          throw std::runtime_error("Unsupported command type: " + type);
        }
      } catch (const std::exception& ex) {
        response["status"] = "ERROR";
        response["message"] = ex.what();
        SPDLOG_WARN("Failed to process command: {}", ex.what());
      }
      srv.sendResponse(response);
    }
  }
}

[[noreturn]] int main(int argc, char** argv) {
  configureLogger();

  argparse::ArgumentParser program("rimokunControl");
  program.add_argument("-c", "--config")
      .help("Path to the config file")
      .default_value("/etc/rimokunControl.yaml");

  try {
    program.parse_args(argc, argv);
  }
  catch (const std::exception& err) {
    SPDLOG_CRITICAL("{}", err.what());
    std::exit(1);
  }

  const auto configPath = program.get<std::string>("-c");

  if (!program.is_used("-c")) {
    SPDLOG_WARN("Config path not provided, using default: {}", configPath);
  }

  if (!fs::exists(configPath)) {
    SPDLOG_CRITICAL("Config file '{}' not found! Exiting.", configPath);
    std::exit(1);
  }

  Config::instance().setConfigPath(configPath);

  auto srv = RimoServer<RobotStatus>();
  std::thread commandThread{&handleCommands, std::ref(srv)};

  while (true) {
    srv.publish(prepareFakeStatus());
    std::this_thread::sleep_for(100ms);
  }
  commandThread.join();
}
