#pragma once

#include <QMainWindow>
#include <memory>
#include <vector>

#include "CommonDefinitions.hpp"
#include "ContecPanelWindow.hpp"
#include "GuiStateStore.hpp"
#include "JoystickPanelWindow.h"
#include "MotorPanelWindow.hpp"
#include "MotorStats.hpp"
#include "MotorStatsPresenter.hpp"
#include "ResetControlsPresenter.hpp"
#include "ToolChangerPresenter.hpp"
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
  void openMotorPanel();
  void openMotorPanelForMotor(utl::EMotor motor);
  void openContecPanel();
  void onJoystickUpdate(int id, double x, double y, bool pressed);

 private:
  void onRobotStatusUpdate(const utl::RobotStatus& status);
  Updater _updater;
  GuiStateStore _stateStore;
  Ui::MainWindow *_ui;
  MotorStatsMap_t _motorStats;
  std::vector<std::unique_ptr<MotorStatsPresenter>> _motorPresenters;
  std::unique_ptr<ToolChangerPresenter> _leftToolChangerPresenter;
  std::unique_ptr<ToolChangerPresenter> _rightToolChangerPresenter;
  std::unique_ptr<ResetControlsPresenter> _resetControlsPresenter;
  std::shared_ptr<spdlog::logger> _previousDefaultLogger;
  std::shared_ptr<spdlog::logger> _logger;
  std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> _consoleSink;
  std::shared_ptr<QtLogSink> _qtSink;
  JoystickPanelWindow* joystickPanel = nullptr;
  MotorPanelWindow* motorPanel = nullptr;
  ContecPanelWindow* contecPanel = nullptr;

};
