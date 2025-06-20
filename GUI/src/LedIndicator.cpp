#include "LedIndicator.hpp"

#include <QPainter>
#include <QTimer>

LedIndicator::LedIndicator(QWidget* parent)
    : QWidget(parent), m_state(utl::ELEDState::Off) {
  setMinimumSize(16, 16);
  s_blinkingLeds.append(this);
}
LedIndicator::operator bool() const { return m_state == utl::ELEDState::On; }
LedIndicator::~LedIndicator() {
  s_blinkingLeds.removeAll(this);
}
void LedIndicator::initBlinkTimer() {
  s_blinkTimer=std::make_unique<QTimer>();
  s_blinkTimer->setInterval(500);
  connect(s_blinkTimer.get(), &QTimer::timeout, toggleBlinkVisibility);
  s_blinkTimer->start();
}
void LedIndicator::toggleBlinkVisibility() {
  s_globalBlinkState = !s_globalBlinkState;
  for (LedIndicator* led : s_blinkingLeds) {
    if (led->m_state == utl::ELEDState::ErrorBlinking) {
      led->m_blinkVisible = s_globalBlinkState;
      led->update();
    }
  }
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
    case utl::ELEDState::ErrorBlinking:
      return  m_blinkVisible ? Qt::red : Qt::transparent;
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
