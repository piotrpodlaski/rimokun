#include "JoystickWidget.h"

#include <QPaintEvent>
#include <QPainter>
#include <QSizePolicy>
#include <algorithm>

JoystickWidget::JoystickWidget(QWidget* parent) : QWidget(parent) {
  setMinimumSize(220, 220);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void JoystickWidget::setXy(double x, double y) {
  const double nx = clampUnit(x);
  const double ny = clampUnit(y);
  if (_x == nx && _y == ny) {
    return;
  }
  _x = nx;
  _y = ny;
  update();
}

void JoystickWidget::setButton(bool pressed) {
  if (_pressed == pressed) {
    return;
  }
  _pressed = pressed;
  update();
}

void JoystickWidget::paintEvent(QPaintEvent* event) {
  QWidget::paintEvent(event);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  const QPalette pal = palette();
  const QColor panel = pal.color(QPalette::Window);
  const QColor line = pal.color(QPalette::WindowText);
  const QColor accent = pal.color(QPalette::Highlight);
  const QColor ringColor = _pressed ? QColor(Qt::red) : line;
  const QColor knobColor = pal.color(QPalette::Highlight);

  p.fillRect(rect(), panel);

  const int side = std::min(width(), height());
  const double radius = side * 0.38;
  const QPointF c(width() * 0.5, height() * 0.5);

  QPen crossPen(line);
  crossPen.setWidthF(1.25);
  p.setPen(crossPen);
  p.drawLine(QPointF(c.x() - radius, c.y()), QPointF(c.x() + radius, c.y()));
  p.drawLine(QPointF(c.x(), c.y() - radius), QPointF(c.x(), c.y() + radius));

  QPen ringPen(ringColor);
  ringPen.setWidthF(2.0);
  p.setPen(ringPen);
  p.setBrush(Qt::NoBrush);
  p.drawEllipse(c, radius, radius);

  const QPointF knobCenter(c.x() + _x * radius, c.y() - _y * radius);
  const double knobRadius = side * 0.09;
  p.setPen(QPen(accent, 1.5));
  p.setBrush(knobColor);
  p.drawEllipse(knobCenter, knobRadius, knobRadius);

  p.setPen(line);
  const QString values = QString("X: %1\nY: %2")
                             .arg(_x, 0, 'f', 3)
                             .arg(_y, 0, 'f', 3);
  p.drawText(rect().adjusted(8, 8, -8, -8), Qt::AlignTop | Qt::AlignLeft, values);
}

double JoystickWidget::clampUnit(double value) {
  return std::clamp(value, -1.0, 1.0);
}
