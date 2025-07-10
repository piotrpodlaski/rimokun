#pragma once

#include <QColor>
#include <QGraphicsObject>
#include <QGraphicsRectItem>
#include <QPropertyAnimation>

class RobotAluBeam final : public QObject, public QGraphicsRectItem {
  Q_OBJECT
  Q_PROPERTY(QPointF pos READ pos WRITE setPos)

 public:
  RobotAluBeam(const QRectF& rect, const QColor& color,
               QGraphicsItem* parent = nullptr)
      : QGraphicsRectItem(rect, parent) {
    setBrush(color);
  }

 public slots:
  void moveTo(const QPointF& target) {
    auto* anim = new QPropertyAnimation(this, "pos");
    anim->setDuration(50);
    anim->setEndValue(target);
    anim->setEasingCurve(QEasingCurve::InOutQuad);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
  }
};
