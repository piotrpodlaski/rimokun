#include <Config.hpp>
#include <Logger.hpp>
#include <MotorControl.hpp>
#include <JsonExtensions.hpp>
#include <magic_enum/magic_enum.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace {
std::atomic_bool gStopRequested{false};

void signalHandler(int) { gStopRequested.store(true, std::memory_order_release); }

struct Args {
  std::string configPath{"Config/rimokun.yaml"};
  std::string motorName{};
  std::int32_t speed1{200};
  std::int32_t speed2{400};
  int runSeconds{5};
  bool listMotorsOnly{false};
};

void printUsage(const char* argv0) {
  std::cout
      << "Usage: " << argv0
      << " [--config PATH] [--motor NAME] [--speed1 N] [--speed2 N] [--run-seconds N] [--list-motors]\n"
      << "Example: " << argv0
      << " --config Config/rimokun.yaml --motor XLeft --speed1 200 --speed2 600 --run-seconds 5\n";
}

bool parseInt(const std::string& s, std::int32_t& out) {
  try {
    std::size_t idx = 0;
    const auto v = std::stoi(s, &idx);
    if (idx != s.size()) return false;
    out = v;
    return true;
  } catch (...) {
    return false;
  }
}

bool parseArgs(int argc, char** argv, Args& args) {
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    const auto needValue = [&](const char* name) {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << name << "\n";
        return false;
      }
      return true;
    };

    if (a == "--help" || a == "-h") {
      printUsage(argv[0]);
      return false;
    }
    if (a == "--list-motors") {
      args.listMotorsOnly = true;
      continue;
    }
    if (a == "--config") {
      if (!needValue("--config")) return false;
      args.configPath = argv[++i];
      continue;
    }
    if (a == "--motor") {
      if (!needValue("--motor")) return false;
      args.motorName = argv[++i];
      continue;
    }
    if (a == "--speed1") {
      if (!needValue("--speed1")) return false;
      if (!parseInt(argv[++i], args.speed1)) {
        std::cerr << "Invalid --speed1 value\n";
        return false;
      }
      continue;
    }
    if (a == "--speed2") {
      if (!needValue("--speed2")) return false;
      if (!parseInt(argv[++i], args.speed2)) {
        std::cerr << "Invalid --speed2 value\n";
        return false;
      }
      continue;
    }
    if (a == "--run-seconds") {
      if (!needValue("--run-seconds")) return false;
      std::int32_t tmp = 0;
      if (!parseInt(argv[++i], tmp) || tmp < 1) {
        std::cerr << "Invalid --run-seconds value\n";
        return false;
      }
      args.runSeconds = tmp;
      continue;
    }

    std::cerr << "Unknown argument: " << a << "\n";
    return false;
  }
  return true;
}

template <typename T>
void printFlags(const std::string& label, const T& status, const std::string& rawLabel,
                const std::uint32_t raw) {
  std::cout << label << " " << rawLabel << "=0x" << std::hex << raw << std::dec << " [";
  for (std::size_t i = 0; i < status.activeFlags.size(); ++i) {
    if (i) std::cout << ", ";
    std::cout << status.activeFlags[i];
  }
  std::cout << "]\n";
}

template <typename TAssignment>
void printAssignments(const char* label, const std::vector<TAssignment>& assignments,
                      const bool inputSide, const MotorRegisterMap& map) {
  auto resolveRegisterInfo = [inputSide, &map](const std::string& channel)
      -> std::pair<std::string, std::string> {
    if (!inputSide) {
      if (channel.rfind("OUT", 0) == 0) {
        const auto index = std::stoi(channel.substr(3));
        const auto upper = map.outputFunctionSelectBase + (2 * index);
        const auto lower = upper + 1;
        if (index > 5) {
          return {std::format("assign=0x{:04X}/0x{:04X}", upper, lower),
                  "state=n/a"};
        }
        return {
            std::format("assign=0x{:04X}/0x{:04X}", upper, lower),
            std::format("state=0x00D4.bit{}", index)};
      }
      if (channel == "MB") {
        return {"assign=fixed", "state=0x00D4.bit8"};
      }
      return {"assign=n/a", "state=n/a"};
    }

    if (channel.rfind("IN", 0) == 0) {
      const auto index = std::stoi(channel.substr(2));
      const auto upper = map.inputFunctionSelectBase + (2 * index);
      const auto lower = upper + 1;
      constexpr std::array<int, 8> kInBitsByChannel = {6, 7, 8, 9, 10, 11, 12, 13};
      if (index >= 0 && index < static_cast<int>(kInBitsByChannel.size())) {
        return {
            std::format("assign=0x{:04X}/0x{:04X}", upper, lower),
            std::format("state=0x00D5.bit{}", kInBitsByChannel[index])};
      }
      return {std::format("assign=0x{:04X}/0x{:04X}", upper, lower), "state=n/a"};
    }
    if (channel == "+LS") return {"assign=fixed", "state=0x00D5.bit0"};
    if (channel == "-LS") return {"assign=fixed", "state=0x00D5.bit1"};
    if (channel == "HOMES") return {"assign=fixed", "state=0x00D5.bit2"};
    if (channel == "SLIT") return {"assign=fixed", "state=0x00D5.bit3"};
    return {"assign=n/a", "state=n/a"};
  };

  std::cout << label << ":\n";
  for (const auto& assignment : assignments) {
    const auto [assignReg, stateReg] = resolveRegisterInfo(assignment.channel);
    std::cout << "  "
              << "[" << (assignment.active ? "1" : "0") << "] "
              << assignment.channel << " -> " << assignment.function
              << " (code=" << assignment.functionCode << ", " << assignReg
              << ", " << stateReg << ")\n";
  }
}

void sleepOrStop(const std::chrono::steady_clock::time_point until) {
  using namespace std::chrono_literals;
  while (!gStopRequested.load(std::memory_order_acquire) &&
         std::chrono::steady_clock::now() < until) {
    std::this_thread::sleep_for(50ms);
  }
}

std::optional<utl::EMotor> resolveMotor(const std::string& name,
                                        const std::vector<utl::EMotor>& configured) {
  if (configured.empty()) {
    return std::nullopt;
  }
  if (name.empty()) {
    return configured.front();
  }
  const auto parsed = magic_enum::enum_cast<utl::EMotor>(name);
  if (!parsed) {
    return std::nullopt;
  }
  const auto it = std::find(configured.begin(), configured.end(), *parsed);
  if (it == configured.end()) {
    return std::nullopt;
  }
  return *parsed;
}
}  // namespace

int main(int argc, char** argv) {
  using namespace std::chrono;
  utl::configureLogger();
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  Args args;
  if (!parseArgs(argc, argv, args)) {
    return 1;
  }

  if (!std::filesystem::exists(args.configPath)) {
    SPDLOG_CRITICAL("Config file '{}' not found", args.configPath);
    return 1;
  }
  utl::Config::instance().setConfigPath(args.configPath);

  try {
    MotorControl mc;
    auto cleanup = [&mc](const std::optional<utl::EMotor> motor) {
      if (motor.has_value()) {
        try {
          mc.stopMovement(*motor);
        } catch (const std::exception& e) {
          SPDLOG_WARN("Failed to stop motor during cleanup: {}", e.what());
        }
      }
      try {
        mc.reset();
      } catch (const std::exception& e) {
        SPDLOG_WARN("Failed to reset MotorControl during cleanup: {}", e.what());
      }
    };

    mc.initialize();
    const auto configuredMotors = mc.configuredMotorIds();
    if (configuredMotors.empty()) {
      SPDLOG_CRITICAL("No motors configured in MotorControl.motors.");
      cleanup(std::nullopt);
      return 1;
    }

    SPDLOG_INFO("Configured motors ({}):", configuredMotors.size());
    for (const auto motor : configuredMotors) {
      SPDLOG_INFO("  {}", utl::enumToString(motor));
    }
    if (args.listMotorsOnly) {
      cleanup(std::nullopt);
      return 0;
    }

    const auto motorOpt = resolveMotor(args.motorName, configuredMotors);
    if (!motorOpt) {
      SPDLOG_CRITICAL(
          "Invalid or unconfigured motor '{}'. Use --list-motors to inspect available motors.",
          args.motorName);
      cleanup(std::nullopt);
      return 1;
    }
    const auto motorId = *motorOpt;
    SPDLOG_INFO("Using motor {}", utl::enumToString(motorId));

    const auto alarm = mc.diagnoseCurrentAlarm(motorId);
    const auto warning = mc.diagnoseCurrentWarning(motorId);
    const auto comm = mc.diagnoseCurrentCommunicationError(motorId);
    SPDLOG_INFO("Current alarm: {}",
                alarm.code == 0
                    ? "none"
                    : std::format("code=0x{:02X}, type='{}'", alarm.code,
                                  alarm.type));
    SPDLOG_INFO("Current warning: {}",
                warning.code == 0
                    ? "none"
                    : std::format("code=0x{:02X}, type='{}'", warning.code,
                                  warning.type));
    SPDLOG_INFO("Current comm error code=0x{:02X}, type='{}'", comm.code, comm.type);

    mc.setMode(motorId, MotorControlMode::Speed);
    mc.setDirection(motorId, MotorControlDirection::Forward);
    mc.setSpeed(motorId, args.speed1);
    mc.startMovement(motorId);
    SPDLOG_INFO("Started speed mode: forward, speed={}", args.speed1);

    const auto total = seconds{std::max(1, args.runSeconds)};
    const auto t0 = steady_clock::now();
    const auto t1 = t0 + total / 3;
    const auto t2 = t0 + (2 * total) / 3;
    const auto tEnd = t0 + total;

    sleepOrStop(t1);
    if (!gStopRequested.load(std::memory_order_acquire)) {
      mc.setSpeed(motorId, args.speed2);
      SPDLOG_INFO("Updated speed while moving: {}", args.speed2);
    }

    sleepOrStop(t2);
    if (!gStopRequested.load(std::memory_order_acquire)) {
      mc.setDirection(motorId, MotorControlDirection::Reverse);
      SPDLOG_INFO("Direction changed to reverse");
    }

    sleepOrStop(tEnd);
    if (!gStopRequested.load(std::memory_order_acquire)) {
      mc.setDirection(motorId, MotorControlDirection::Forward);
      SPDLOG_INFO("Direction changed to forward");
    }

    const auto input = mc.readInputStatus(motorId);
    const auto output = mc.readOutputStatus(motorId);
    const auto dio = mc.readDirectIoStatus(motorId);
    const auto& motorMap = mc.motors().at(motorId).map();
    printFlags("Input", input, "raw", input.raw);
    printFlags("Output", output, "raw", output.raw);
    std::cout << "DirectIO 00D4=0x" << std::hex << dio.reg00D4 << " 00D5=0x"
              << dio.reg00D5 << std::dec << "\n";
    printAssignments("Input assignments", dio.inputAssignments, true, motorMap);
    printAssignments("Output assignments", dio.outputAssignments, false, motorMap);

    cleanup(motorId);
    if (gStopRequested.load(std::memory_order_acquire)) {
      SPDLOG_WARN("Interrupted by signal; exited gracefully.");
    }
    return 0;
  } catch (const std::exception& e) {
    SPDLOG_CRITICAL("motorControlDemo failed: {}", e.what());
    return 2;
  }
}
