#include "RobotVisualisation.hpp"

#include <QGraphicsView>
#include <QVBoxLayout>
#include <optional>

#include "Config.hpp"
#include "magic_enum/magic_enum.hpp"
#include "spdlog/spdlog.h"

RobotVisualisation::RobotVisualisation(QWidget* parent) : QWidget(parent) {
  auto layout = new QVBoxLayout(this);
  _view = std::make_unique<QGraphicsView>();
  layout->addWidget(_view.get());

  const auto configNode =
      utl::Config::instance().getClassConfig("RobotVisualisation");

  _scene = new QGraphicsScene(this);
  _scene->setSceneRect(0, 0, configNode["scene"]["width"].as<int>(),
                       configNode["scene"]["height"].as<int>());
  _view->setRenderHint(QPainter::Antialiasing);
  _view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  _view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  _view->setScene(_scene);

  auto topNode = configNode["beams"]["top"];
  auto leftNode = configNode["beams"]["left"];
  auto rightNode = configNode["beams"]["right"];

  QColor topColor(QString::fromStdString(topNode["color"].as<std::string>()));
  QColor leftColor(QString::fromStdString(leftNode["color"].as<std::string>()));
  QColor rightColor(
      QString::fromStdString(rightNode["color"].as<std::string>()));

  _topBeam = std::make_unique<RobotAluBeam>(
      QRectF(0, 0, topNode["width"].as<int>(), topNode["height"].as<int>()),
      topColor);
  _leftBeam = std::make_unique<RobotAluBeam>(
      QRectF(0, 0, leftNode["width"].as<int>(), leftNode["height"].as<int>()),
      leftColor);
  _rightBeam = std::make_unique<RobotAluBeam>(
      QRectF(0, 0, rightNode["width"].as<int>(), rightNode["height"].as<int>()),
      rightColor);

  _topBeam->setPos(50, 50);
  _rightBeam->setPos(0, 50);
  _leftBeam->setPos(150, 50);

  _scene->addItem(_topBeam.get());
  _scene->addItem(_rightBeam.get());
  _scene->addItem(_leftBeam.get());
  _scene->addRect(_scene->sceneRect(), QPen(Qt::black), Qt::NoBrush);
  _view->fitInView(_scene->sceneRect(), Qt::KeepAspectRatio);
}

void RobotVisualisation::moveTop(const QPointF& pos) const {
  _topBeam->moveTo(pos);
}

void RobotVisualisation::moveLeft(const QPointF& pos) const {
  _leftBeam->moveTo(pos);
}

void RobotVisualisation::moveRight(const QPointF& pos) const {
  _rightBeam->moveTo(pos);
}

void RobotVisualisation::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  _view->fitInView(_scene->sceneRect(), Qt::KeepAspectRatio);
}

void RobotVisualisation::updateRobotStatus(const utl::RobotStatus& rs) const {
  const auto updateArm = [&rs](utl::EMotor xMotor, utl::EMotor yMotor)
      -> std::optional<QPointF> {
    const auto xIt = rs.motors.find(xMotor);
    const auto yIt = rs.motors.find(yMotor);
    if (xIt == rs.motors.end() || yIt == rs.motors.end()) {
      SPDLOG_WARN("Robot status lacks data for {} or {}. Skipping update.",
                  magic_enum::enum_name(xMotor),
                  magic_enum::enum_name(yMotor));
      return std::nullopt;
    }
    return QPointF{xIt->second.currentPosition,
                   yIt->second.currentPosition};
  };

  if (const auto leftPos =
          updateArm(utl::EMotor::XLeft, utl::EMotor::YLeft)) {
    moveLeft(*leftPos);
  }

  if (const auto rightPos =
          updateArm(utl::EMotor::XRight, utl::EMotor::YRight)) {
    moveRight(*rightPos);
  }
}
