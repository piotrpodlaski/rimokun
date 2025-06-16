//
// Created by piotrek on 6/6/25.
//

// You may need to build the project (run Qt uic code generator) to get
// "ui_MainWindow.h" resolved

#include "MainWindow.hpp"

#include <QTimer>
#include <print>

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
  updater.startUpdaterThread();

  connect(&updater, &Updater::newDataArrived, ui->widget, &RobotVisualisation::updateRobotPosition);
}

MainWindow::~MainWindow() { delete ui; }
