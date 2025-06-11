#pragma once

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QWidget>

#include "RobotAluBeam.hpp"

class RobotVisualisation : public QWidget {
  Q_OBJECT

 public:
  explicit RobotVisualisation(QWidget* parent = nullptr);

 public slots:
  void moveRedTo(const QPointF& pos) const;
  void moveGreenTo(const QPointF& pos) const;
  void moveBlueTo(const QPointF& pos) const;

 private:
  QGraphicsScene* scene_;
  RobotAluBeam* red_;
  RobotAluBeam* green_;
  RobotAluBeam* blue_;
};
