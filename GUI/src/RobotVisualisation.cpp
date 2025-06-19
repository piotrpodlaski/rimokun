#include "RobotVisualisation.hpp"

#include <QGraphicsView>
#include <QVBoxLayout>
#include <iostream>

#include "ClassName.hpp"
#include "Config.hpp"
#include "spdlog/spdlog.h"

RobotVisualisation::RobotVisualisation(QWidget* parent) : QWidget(parent) {
  auto layout = new QVBoxLayout(this);
  view_ = new QGraphicsView;
  layout->addWidget(view_);

  const auto configNode = utl::Config::instance().getConfig(this);

  std::cout << configNode << std::endl;

  scene_ = new QGraphicsScene(this);
  scene_->setSceneRect(0, 0, configNode["scene"]["width"].as<int>(),
                       configNode["scene"]["height"].as<int>());
  view_->setRenderHint(QPainter::Antialiasing);
  view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view_->setScene(scene_);

  auto topNode = configNode["beams"]["top"];
  auto leftNode = configNode["beams"]["left"];
  auto rightNode = configNode["beams"]["right"];

  QColor topColor(QString::fromStdString(topNode["color"].as<std::string>()));
  QColor leftColor(QString::fromStdString(leftNode["color"].as<std::string>()));
  QColor rightColor(
      QString::fromStdString(rightNode["color"].as<std::string>()));

  blue_ = new RobotAluBeam(
      QRectF(0, 0, topNode["width"].as<int>(), topNode["height"].as<int>()),
      topColor);
  red_ = new RobotAluBeam(
      QRectF(0, 0, leftNode["width"].as<int>(), leftNode["height"].as<int>()),
      leftColor);
  green_ = new RobotAluBeam(
      QRectF(0, 0, rightNode["width"].as<int>(), rightNode["height"].as<int>()),
      rightColor);

  blue_->setPos(0, 50);
  red_->setPos(50, 50);
  green_->setPos(150, 50);

  scene_->addItem(blue_);
  scene_->addItem(red_);
  scene_->addItem(green_);
  scene_->addRect(scene_->sceneRect(), QPen(Qt::black), Qt::NoBrush);
  view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
}

void RobotVisualisation::moveRedTo(const QPointF& pos) const {
  red_->moveTo(pos);
}

void RobotVisualisation::moveGreenTo(const QPointF& pos) const {
  green_->moveTo(pos);
}

void RobotVisualisation::moveBlueTo(const QPointF& pos) const {
  blue_->moveTo(pos);
}

void RobotVisualisation::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  // scene_->setSceneRect(0, 0, view_->viewport()->width(),
  // view_->viewport()->height());
  view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
}

void RobotVisualisation::updateRobotStatus(const utl::RobotStatus& rs) const {
  auto xL = rs.motors.at(utl::EMotor::XLeft).currentPosition;
  auto yL = rs.motors.at(utl::EMotor::YLeft).currentPosition;
  moveGreenTo({xL, yL});

  auto xR = rs.motors.at(utl::EMotor::XRight).currentPosition;
  auto yR = rs.motors.at(utl::EMotor::YRight).currentPosition;
  moveRedTo({xR, yR});
}
