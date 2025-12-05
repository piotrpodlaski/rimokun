
#include "Config.hpp"
#include "Logger.hpp"
#include "Machine.hpp"
#include "argparse/argparse.hpp"
#include <atomic>
#include <csignal>
#include <chrono>

using namespace std::chrono_literals;

std::atomic running{true};

void signalHandler(int) {
  running = false;
}

using namespace utl;
namespace fs = std::filesystem;

int main(int argc, char** argv) {

  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler); // nice for systemd too

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

  Machine m;
  m.initialize();

  while (running) {
    std::this_thread::sleep_for(100ms);
  }

  m.shutdown();


}
