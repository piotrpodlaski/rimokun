#include "LedIndicator.hpp"

#include <QPainter>

LedIndicator::LedIndicator(QWidget* parent)
    : QWidget(parent), m_state(State::Off) {
  setMinimumSize(16, 16);
}

void LedIndicator::setState(const State state) {
  if (m_state != state) {
    m_state = state;
    update();  // trigger repaint
  }
}

LedIndicator::State LedIndicator::state() const { return m_state; }

QColor LedIndicator::currentColor() const {
  switch (m_state) {
    case State::On:
      return Qt::green;
    case State::Error:
      return Qt::red;
    default:
      return Qt::darkYellow;
  }
}

void LedIndicator::paintEvent(QPaintEvent*) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const QColor color = currentColor();
  painter.setBrush(color);
  painter.setPen(Qt::black);

  const int size = qMin(width(), height()) - 2;
  painter.drawEllipse((width() - size) / 2, (height() - size) / 2, size, size);
}

QSize LedIndicator::sizeHint() const { return QSize(24, 24); }
