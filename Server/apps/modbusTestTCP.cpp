#include <iostream>
#include <thread>
#include <chrono>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>


#include "ModbusClient.hpp"
#include "Contec.hpp"
#include "Machine.hpp"
#include "Logger.hpp"
#include "Config.hpp"

#include "argparse/argparse.hpp"

using namespace std::chrono_literals;
namespace fs = std::filesystem;

int main(int argc, char**argv) {
  utl::configureLogger();
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

  utl::Config::instance().setConfigPath(configPath);

  Contec c;

  c.setOutputs({0,1,1,1,1,1,1,1});

  std::vector<bool> outputs(8,false);
  while (true) {
    auto inputs = c.readInputs();
    outputs[2] = inputs[9];
    outputs[3] = inputs[10];
    c.setOutputs(outputs);
    std::this_thread::sleep_for(10ms);

  }

  return 0;

}
