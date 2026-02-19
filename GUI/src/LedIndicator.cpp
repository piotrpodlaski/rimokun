#include "LedIndicator.hpp"

#include <QPainter>
#include <QTimer>

LedIndicator::LedIndicator(QWidget* parent)
    : QWidget(parent), _ledState(utl::ELEDState::Off) {
  setMinimumSize(16, 16);
  _blinkingLeds.append(this);
}
LedIndicator::operator bool() const { return _ledState == utl::ELEDState::On; }
LedIndicator::~LedIndicator() {
  _blinkingLeds.removeAll(this);
}
void LedIndicator::initBlinkTimer() {
  _blinkTimer=std::make_unique<QTimer>();
  _blinkTimer->setInterval(500);
  connect(_blinkTimer.get(), &QTimer::timeout, toggleBlinkVisibility);
  _blinkTimer->start();
}
void LedIndicator::toggleBlinkVisibility() {
  _globalBlinkState = !_globalBlinkState;
  for (LedIndicator* led : _blinkingLeds) {
    if (led->_ledState == utl::ELEDState::ErrorBlinking) {
      led->_blinkVisible = _globalBlinkState;
      led->update();
    }
  }
}

void LedIndicator::setState(const utl::ELEDState state) {
  if (_ledState != state) {
    _ledState = state;
    update();  // trigger repaint
  }
}


utl::ELEDState LedIndicator::state() const { return _ledState; }

QColor LedIndicator::currentColor() const {
  switch (_ledState) {
    case utl::ELEDState::On:
      return Qt::green;
    case utl::ELEDState::Warning:
      return QColor(255, 165, 0);
    case utl::ELEDState::Error:
      return Qt::red;
    case utl::ELEDState::Off:
      return Qt::transparent;
    case utl::ELEDState::ErrorBlinking:
      return  _blinkVisible ? Qt::red : Qt::transparent;
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

QSize LedIndicator::sizeHint() const { return {24, 24}; }
