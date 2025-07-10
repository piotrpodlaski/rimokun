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
  void moveTop(const QPointF& pos) const;
  void moveLeft(const QPointF& pos) const;
  void moveRight(const QPointF& pos) const;
  void updateRobotStatus(const utl::RobotStatus& rs) const;


 private:
  QGraphicsScene* _scene;
  std::unique_ptr<RobotAluBeam> _topBeam;
  std::unique_ptr<RobotAluBeam> _leftBeam;
  std::unique_ptr<RobotAluBeam> _rightBeam;
  std::unique_ptr<QGraphicsView> _view;
};
