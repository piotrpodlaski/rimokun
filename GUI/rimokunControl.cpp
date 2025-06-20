#include <QApplication>
#include <logger.hpp>

#include "MainWindow.hpp"
#include "argparse/argparse.hpp"
#include "Config.hpp"

#include <filesystem>
namespace fs = std::filesystem;


int main(int argc, char* argv[]) {

  utl::configure_logger();
  argparse::ArgumentParser program("rimokunControl");
  program.add_argument("-c", "--config")
      .help("PAth to the config file")
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

  QApplication a(argc, argv);
  QApplication::setStyle("Fusion");
  MainWindow w;
  w.show();
  const auto retCode = QApplication::exec();
  return retCode;
}