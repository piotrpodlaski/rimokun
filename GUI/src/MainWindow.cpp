//
// Created by piotrek on 6/6/25.
//

// You may need to build the project (run Qt uic code generator) to get
// "ui_MainWindow.h" resolved

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  motorStats[utl::EMotor::XLeft] = ui->mot1;
  motorStats[utl::EMotor::XRight] = ui->mot3;
  motorStats[utl::EMotor::YLeft] = ui->mot2;
  motorStats[utl::EMotor::YRight] = ui->mot4;
  motorStats[utl::EMotor::ZLeft] = ui->mot5;
  motorStats[utl::EMotor::ZRight] = ui->mot6;

  for (auto& [eMot, motStat] : motorStats) {
    motStat->setMotorId(eMot);
    motStat->setCurrentPosition(4010.2);
    motStat->setSpeed(2137);
    motStat->setTargetPosition(420);
    motStat->setTorque(69);
    motStat->setBrake(utl::ELEDState::Off);
    motStat->setEnabled(utl::ELEDState::Error);
    connect(&updater, &Updater::newDataArrived,
            dynamic_cast<MotorStats*>(motStat), &MotorStats::handleUpdate);
  }
  const auto leftChanger = ui->leftChanger;
  const auto rightChanger = ui->rightChanger;
  leftChanger->setArm(utl::EArm::Left);
  rightChanger->setArm(utl::EArm::Right);
  const auto robotVis = ui->widget;
  updater.startUpdaterThread();

  connect(&updater, &Updater::newDataArrived, robotVis, &RobotVisualisation::updateRobotStatus);
  connect(&updater, &Updater::newDataArrived, leftChanger, &ToolChanger::updateRobotStatus);
  connect(&updater, &Updater::newDataArrived, rightChanger, &ToolChanger::updateRobotStatus);

  connect(leftChanger, &ToolChanger::buttonPressed, &updater, &Updater::sendCommand);
  connect(rightChanger, &ToolChanger::buttonPressed, &updater, &Updater::sendCommand);

}

MainWindow::~MainWindow() { delete ui; }
