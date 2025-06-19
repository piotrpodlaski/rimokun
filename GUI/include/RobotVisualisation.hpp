#pragma once

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QWidget>
#include "CommonDefinitions.hpp"

#include "RobotAluBeam.hpp"

class RobotVisualisation final : public QWidget {
  Q_OBJECT

 public:
  explicit RobotVisualisation(QWidget* parent = nullptr);

  void resizeEvent(QResizeEvent* event) override;

 public slots:
  void moveRedTo(const QPointF& pos) const;
  void moveGreenTo(const QPointF& pos) const;
  void moveBlueTo(const QPointF& pos) const;
  void updateRobotStatus(const utl::RobotStatus& rs) const;


 private:
  QGraphicsScene* scene_;
  RobotAluBeam* red_;
  RobotAluBeam* green_;
  RobotAluBeam* blue_;
  QGraphicsView* view_;
};
