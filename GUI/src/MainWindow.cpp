#include "MainWindow.hpp"

#include <Logger.hpp>

#include "LedIndicator.hpp"
#include "QtLogSink.hpp"
#include "TitleBar.hpp"
#include "spdlog/spdlog.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), _ui(new Ui::MainWindow) {
  _ui->setupUi(this);

  _motorStats[utl::EMotor::XLeft] = _ui->mot1;
  _motorStats[utl::EMotor::XRight] = _ui->mot3;
  _motorStats[utl::EMotor::YLeft] = _ui->mot2;
  _motorStats[utl::EMotor::YRight] = _ui->mot4;
  _motorStats[utl::EMotor::ZLeft] = _ui->mot5;
  _motorStats[utl::EMotor::ZRight] = _ui->mot6;

  setWindowTitle("Rimo-kun Control");
  // setWindowIcon(QIcon(":/resources/rimoKunLogo.png"));

  for (auto& [eMot, motStat] : _motorStats) {
    motStat->setCurrentPosition(4010.2);
    motStat->setSpeed(2137);
    motStat->setTargetPosition(420);
    motStat->setTorque(69);
    motStat->setBrake(utl::ELEDState::Off);
    motStat->setEnabled(utl::ELEDState::Error);
    _motorPresenters.push_back(
        std::make_unique<MotorStatsPresenter>(
            dynamic_cast<MotorStats*>(motStat), eMot, &_stateStore, this));
  }
  const auto leftChanger = _ui->leftChanger;
  const auto rightChanger = _ui->rightChanger;
  const auto robotVis = _ui->robotVis;

  _leftToolChangerPresenter = std::make_unique<ToolChangerPresenter>(
      leftChanger, utl::EArm::Left, &_stateStore, this);
  _rightToolChangerPresenter = std::make_unique<ToolChangerPresenter>(
      rightChanger, utl::EArm::Right, &_stateStore, this);
  _resetControlsPresenter = std::make_unique<ResetControlsPresenter>(
      _ui->resetControls, &_stateStore, this);

  connect(_leftToolChangerPresenter.get(), &ToolChangerPresenter::commandIssued,
          &_updater, &Updater::sendCommand);
  connect(_rightToolChangerPresenter.get(), &ToolChangerPresenter::commandIssued,
          &_updater, &Updater::sendCommand);
  connect(_resetControlsPresenter.get(), &ResetControlsPresenter::commandIssued,
          &_updater, &Updater::sendCommand);

  connect(&_updater, &Updater::newDataArrived, &_stateStore,
          &GuiStateStore::onStatusReceived);
  connect(&_updater, &Updater::serverNotConnected, &_stateStore,
          &GuiStateStore::onServerDisconnected);
  connect(&_stateStore, &GuiStateStore::statusUpdated, robotVis,
          &RobotVisualisation::updateRobotStatus);
  connect(&_updater, &Updater::newDataArrived, this,
          &MainWindow::onRobotStatusUpdate);

  _updater.startUpdaterThread();


  _consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  _qtSink = std::make_shared<QtLogSink>(_ui->logOutput);
  _logger = std::make_shared<spdlog::logger>(
      "gui", spdlog::sinks_init_list{_consoleSink, _qtSink});
  spdlog::register_logger(_logger);
  _previousDefaultLogger = spdlog::default_logger();
  spdlog::set_default_logger(_logger);
  utl::configureLogger();
  LedIndicator::initBlinkTimer();

  auto titleBar = _ui->titleBar;

  titleBar->setLeftLogo(QPixmap(":/resources/rimoKunLogo.png"));
  titleBar->setRightLogo(QPixmap(":/resources/KEKLogo.png"));
  titleBar->setTitleText("Remote Clamp Controller");

  connect(_ui->actionControl_panel_monitor, &QAction::triggered, this,
          &MainWindow::openJoystickPanel);
}

MainWindow::~MainWindow() {
  _updater.stopUpdaterThread();
  if (_previousDefaultLogger) {
    spdlog::set_default_logger(_previousDefaultLogger);
  }
  spdlog::drop("gui");
  _previousDefaultLogger.reset();
  _logger.reset();
  _qtSink.reset();
  _consoleSink.reset();
  delete _ui;
}

void MainWindow::openJoystickPanel() {
  if (joystickPanel == nullptr) {
    joystickPanel = new JoystickPanelWindow(this);
  }
  joystickPanel->show();
  joystickPanel->raise();
  joystickPanel->activateWindow();
}

void MainWindow::onJoystickUpdate(int id, double x, double y, bool pressed) {
  if (joystickPanel) {
    joystickPanel->setState(id, x, y, pressed);
  }
}

void MainWindow::onRobotStatusUpdate(const utl::RobotStatus& status) {
  if (status.joystics.contains(utl::EArm::Left)) {
    const auto& js = status.joystics.at(utl::EArm::Left);
    onJoystickUpdate(0, js.x, js.y, js.btn);
  }
  if (status.joystics.contains(utl::EArm::Right)) {
    const auto& js = status.joystics.at(utl::EArm::Right);
    onJoystickUpdate(1, js.x, js.y, js.btn);
  }
  if (status.joystics.contains(utl::EArm::Gantry)) {
    const auto& js = status.joystics.at(utl::EArm::Gantry);
    onJoystickUpdate(2, js.x, js.y, js.btn);
  }
}
