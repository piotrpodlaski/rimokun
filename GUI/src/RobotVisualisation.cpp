#include "RobotVisualisation.hpp"

#include <QGraphicsView>
#include <QVBoxLayout>

RobotVisualisation::RobotVisualisation(QWidget* parent) : QWidget(parent) {
  auto layout = new QVBoxLayout;
  auto view = new QGraphicsView;
  layout->addWidget(view);
  setLayout(layout);

  scene_ = new QGraphicsScene(this);
  scene_->setSceneRect(0, 0, 400, 300);
  view->setScene(scene_);

  red_ = new RobotAluBeam(QRectF(0, 0, 50, 50), Qt::red);
  green_ = new RobotAluBeam(QRectF(0, 0, 50, 50), Qt::green);
  blue_ = new RobotAluBeam(QRectF(0, 0, 50, 50), Qt::blue);

  red_->setPos(50, 50);
  green_->setPos(150, 50);
  blue_->setPos(250, 50);

  scene_->addItem(red_);
  scene_->addItem(green_);
  scene_->addItem(blue_);
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
