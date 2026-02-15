#pragma once

#include <QMainWindow>

#include "CommonDefinitions.hpp"
#include "JoystickPanelWindow.h"
#include "MotorStats.hpp"
#include "Updater.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "QtLogSink.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;
  void openJoystickPanel();
  void onJoystickUpdate(int id, double x, double y, bool pressed);

 private:
  void onRobotStatusUpdate(const utl::RobotStatus& status);
  Updater _updater;
  Ui::MainWindow *_ui;
  MotorStatsMap_t _motorStats;
  std::shared_ptr<spdlog::logger> _logger;
  std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> _consoleSink;
  std::shared_ptr<QtLogSink> _qtSink;
  JoystickPanelWindow* joystickPanel = nullptr;

};
