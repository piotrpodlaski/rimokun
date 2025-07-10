#include <Logger.hpp>
#include <QApplication>
#include <QDirIterator>
#include <filesystem>

#include "Config.hpp"
#include "MainWindow.hpp"
#include "argparse/argparse.hpp"
namespace fs = std::filesystem;


int main(int argc, char* argv[]) {

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

  QDirIterator it(":", QDirIterator::Subdirectories);

  qDebug() << "Listing Qt Resources:";
  while (it.hasNext()) {
    qDebug() << it.next();
  }

  utl::Config::instance().setConfigPath(configPath);

  QApplication a(argc, argv);
  QApplication::setStyle("Fusion");
  MainWindow w;
  w.show();
  const auto retCode = QApplication::exec();
  return retCode;
}