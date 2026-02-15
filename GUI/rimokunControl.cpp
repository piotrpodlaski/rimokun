#include <Logger.hpp>
#include <QApplication>
#include <QDirIterator>
#include <QTimer>
#include <atomic>
#include <csignal>
#include <filesystem>

#include "Config.hpp"
#include "MainWindow.hpp"
#include "argparse/argparse.hpp"
namespace fs = std::filesystem;

namespace {
std::atomic<bool> g_shutdownRequested{false};

void signalHandler(int) { g_shutdownRequested.store(true, std::memory_order_release); }
}  // namespace


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

  utl::Config::instance().setConfigPath(configPath);

  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  QApplication a(argc, argv);
  QApplication::setStyle("Fusion");

  QTimer shutdownPollTimer;
  QObject::connect(&shutdownPollTimer, &QTimer::timeout, &a, [&a]() {
    if (g_shutdownRequested.load(std::memory_order_acquire)) {
      a.quit();
    }
  });
  shutdownPollTimer.start(50);

  MainWindow w;
  w.show();
  const auto retCode = QApplication::exec();
  return retCode;
}
