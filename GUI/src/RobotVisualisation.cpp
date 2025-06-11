#include "RobotVisualisation.hpp"

#include <QGraphicsView>
#include <QVBoxLayout>

RobotVisualisation::RobotVisualisation(QWidget* parent) : QWidget(parent) {
  auto layout = new QVBoxLayout;
  view_ = new QGraphicsView;
  layout->addWidget(view_);
  setLayout(layout);

  scene_ = new QGraphicsScene(this);
  scene_->setSceneRect(0, 0, 700 , 300);
  view_->setRenderHint(QPainter::Antialiasing);
  view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view_->setScene(scene_);

  blue_ = new RobotAluBeam(QRectF(0, 0, 700, 20), Qt::blue);
  red_ = new RobotAluBeam(QRectF(0, 0, 15, 200), Qt::red);
  green_ = new RobotAluBeam(QRectF(0, 0, 15, 200), Qt::green);

  blue_->setPos(0, 50);
  red_->setPos(50, 50);
  green_->setPos(150, 50);

  scene_->addItem(blue_);
  scene_->addItem(red_);
  scene_->addItem(green_);
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
  //scene_->setSceneRect(0, 0, view_->viewport()->width(), view_->viewport()->height());
  view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
}

void RobotVisualisation::updateRobotPosition(const utl::RobotStatus& rs) const {
  auto xL = rs.motors.at(utl::EMotor::XLeft).currentPosition;
  auto yL = rs.motors.at(utl::EMotor::YLeft).currentPosition;
  moveGreenTo({xL, yL});

  auto xR = rs.motors.at(utl::EMotor::XRight).currentPosition;
  auto yR = rs.motors.at(utl::EMotor::YRight).currentPosition;
  moveRedTo({xR, yR});
}

