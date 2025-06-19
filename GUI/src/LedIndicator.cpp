#include "LedIndicator.hpp"

#include <QPainter>

LedIndicator::LedIndicator(QWidget* parent)
    : QWidget(parent), m_state(utl::ELEDState::Off) {
  setMinimumSize(16, 16);
}
LedIndicator::operator bool() const {
  return m_state == utl::ELEDState::On;
}

void LedIndicator::setState(const utl::ELEDState state) {
  if (m_state != state) {
    m_state = state;
    update();  // trigger repaint
  }
}

utl::ELEDState LedIndicator::state() const { return m_state; }

QColor LedIndicator::currentColor() const {
  switch (m_state) {
    case utl::ELEDState::On:
      return Qt::green;
    case utl::ELEDState::Error:
      return Qt::red;
    case utl::ELEDState::Off:
      return Qt::transparent;
    default:
      return Qt::transparent;
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
