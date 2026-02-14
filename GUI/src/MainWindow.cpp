#include "MainWindow.hpp"

#include "LedIndicator.hpp"
#include "QtLogSink.hpp"
#include "TitleBar.hpp"
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
    motStat->setMotorId(eMot);
    motStat->setCurrentPosition(4010.2);
    motStat->setSpeed(2137);
    motStat->setTargetPosition(420);
    motStat->setTorque(69);
    motStat->setBrake(utl::ELEDState::Off);
    motStat->setEnabled(utl::ELEDState::Error);
    connect(&_updater, &Updater::newDataArrived,
            dynamic_cast<MotorStats*>(motStat), &MotorStats::handleUpdate);
  }
  const auto leftChanger = _ui->leftChanger;
  const auto rightChanger = _ui->rightChanger;
  leftChanger->setArm(utl::EArm::Left);
  rightChanger->setArm(utl::EArm::Right);
  const auto robotVis = _ui->robotVis;
  _updater.startUpdaterThread();

  connect(&_updater, &Updater::newDataArrived, robotVis,
          &RobotVisualisation::updateRobotStatus);
  connect(&_updater, &Updater::newDataArrived, leftChanger,
          &ToolChanger::updateRobotStatus);
  connect(&_updater, &Updater::newDataArrived, rightChanger,
          &ToolChanger::updateRobotStatus);

  connect(leftChanger, &ToolChanger::buttonPressed, &_updater,
          &Updater::sendCommand);
  connect(rightChanger, &ToolChanger::buttonPressed, &_updater,
          &Updater::sendCommand);

  connect(_ui->resetControls, &ResetControls::buttonPressed, &_updater,
          &Updater::sendCommand);

  connect(&_updater, &Updater::newDataArrived, _ui->resetControls,
          &ResetControls::updateRobotStatus);

  connect(&_updater, &Updater::serverNotConnected, _ui->resetControls,
          &ResetControls::announceServerError);


  _consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  _qtSink = std::make_shared<QtLogSink>(_ui->logOutput);
  _logger = std::make_shared<spdlog::logger>(
      "gui", spdlog::sinks_init_list{_consoleSink, _qtSink});
  spdlog::register_logger(_logger);
  spdlog::set_default_logger(_logger);  // optional
  utl::configureLogger();
  LedIndicator::initBlinkTimer();

  auto titleBar = _ui->titleBar;

  titleBar->setLeftLogo(QPixmap(":/resources/rimoKunLogo.png"));
  titleBar->setRightLogo(QPixmap(":/resources/KEKLogo.png"));
  titleBar->setTitleText("Remote Clamp Controller");
}

MainWindow::~MainWindow() { delete _ui; }
