#include <Config.hpp>
#include <Logger.hpp>
#include <MotorControl.hpp>
#include <magic_enum/magic_enum.hpp>

#include <chrono>
#include <algorithm>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <thread>

namespace {
struct Args {
  std::string configPath{"Config/rimokun.yaml"};
  std::string motorName{"XLeft"};
  std::int32_t speed1{200};
  std::int32_t speed2{400};
  int runSeconds{5};
};

void printUsage(const char* argv0) {
  std::cout
      << "Usage: " << argv0
      << " [--config PATH] [--motor NAME] [--speed1 N] [--speed2 N] [--run-seconds N]\n"
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
}  // namespace

int main(int argc, char** argv) {
  utl::configureLogger();

  Args args;
  if (!parseArgs(argc, argv, args)) {
    return 1;
  }

  if (!std::filesystem::exists(args.configPath)) {
    SPDLOG_CRITICAL("Config file '{}' not found", args.configPath);
    return 1;
  }
  utl::Config::instance().setConfigPath(args.configPath);

  const auto motorOpt = magic_enum::enum_cast<utl::EMotor>(args.motorName);
  if (!motorOpt) {
    SPDLOG_CRITICAL("Invalid motor '{}'.", args.motorName);
    return 1;
  }
  const auto motorId = *motorOpt;

  try {
    MotorControl mc;
    mc.initialize();

    // Diagnostics snapshot similar to manual checks in the python script.
    const auto alarm = mc.diagnoseCurrentAlarm(motorId);
    const auto warning = mc.diagnoseCurrentWarning(motorId);
    const auto comm = mc.diagnoseCurrentCommunicationError(motorId);
    SPDLOG_INFO("Current alarm code=0x{:02X}, type='{}'", alarm.code, alarm.type);
    SPDLOG_INFO("Current warning code=0x{:02X}, type='{}'", warning.code, warning.type);
    SPDLOG_INFO("Current comm error code=0x{:02X}, type='{}'", comm.code, comm.type);

    // Mimic python-style speed run: configure speed mode, run, update speed while moving,
    // flip direction on the fly, then stop.
    mc.setMode(motorId, MotorControlMode::Speed);
    mc.setDirection(motorId, MotorControlDirection::Forward);
    mc.setSpeed(motorId, args.speed1);
    mc.startMovement(motorId);
    SPDLOG_INFO("Started motor {} in speed mode, speed={}.", args.motorName,
                args.speed1);

    std::this_thread::sleep_for(std::chrono::seconds{std::max(1, args.runSeconds / 3)});

    mc.setSpeed(motorId, args.speed2);
    SPDLOG_INFO("Updated speed while moving to {} (buffered op switch).", args.speed2);

    std::this_thread::sleep_for(std::chrono::seconds{std::max(1, args.runSeconds / 3)});

    mc.setDirection(motorId, MotorControlDirection::Reverse);
    SPDLOG_INFO("Changed direction on the fly: REVERSE.");

    std::this_thread::sleep_for(
        std::chrono::seconds{std::max(1, args.runSeconds - 2 * std::max(1, args.runSeconds / 3))});

    mc.setDirection(motorId, MotorControlDirection::Forward);
    SPDLOG_INFO("Changed direction on the fly: FORWARD.");

    const auto input = mc.readInputStatus(motorId);
    const auto output = mc.readOutputStatus(motorId);
    const auto dio = mc.readDirectIoStatus(motorId);
    printFlags("Input", input, "raw", input.raw);
    printFlags("Output", output, "raw", output.raw);
    std::cout << "DirectIO 00D4=0x" << std::hex << dio.reg00D4 << " 00D5=0x"
              << dio.reg00D5 << std::dec << " [";
    for (std::size_t i = 0; i < dio.activeFlags.size(); ++i) {
      if (i) std::cout << ", ";
      std::cout << dio.activeFlags[i];
    }
    std::cout << "]\n";

    mc.stopMovement(motorId);
    SPDLOG_INFO("Stopped motor {}.", args.motorName);

    mc.reset();
    return 0;
  } catch (const std::exception& e) {
    SPDLOG_CRITICAL("motorControlDemo failed: {}", e.what());
    return 2;
  }
}
