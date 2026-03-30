#include "SpeedCommandBar.hpp"

#include <QPaintEvent>
#include <QPainter>
#include <QSizePolicy>
#include <algorithm>
#include <cmath>

SpeedCommandBar::SpeedCommandBar(QWidget* parent) : QWidget(parent) {
  setMinimumHeight(26);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void SpeedCommandBar::setValues(const double percent, const double maxMmPerSec) {
  const double clampedPct = std::clamp(percent, -100.0, 100.0);
  if (_percent == clampedPct && _maxMmPerSec == maxMmPerSec) {
    return;
  }
  _percent = clampedPct;
  _maxMmPerSec = maxMmPerSec;
  update();
}

void SpeedCommandBar::paintEvent(QPaintEvent* event) {
  QWidget::paintEvent(event);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, false);

  const QRect r = rect().adjusted(1, 1, -1, -1);
  const int cx = r.left() + r.width() / 2;  // centre x

  // Background
  p.setPen(Qt::NoPen);
  p.setBrush(QColor(0x28, 0x28, 0x28));
  p.drawRoundedRect(r, 3, 3);

  // Fill bar
  if (std::abs(_percent) > 0.5) {
    const int fillW = static_cast<int>(std::abs(_percent) / 100.0 * (r.width() / 2));
    QRect fillRect;
    QColor fillColor;
    if (_percent > 0.0) {
      // Forward: fill from centre to right, green
      fillRect = QRect(cx, r.top(), fillW, r.height());
      fillColor = QColor(0x2e, 0x9e, 0x4e);
    } else {
      // Reverse: fill from centre to left, amber
      fillRect = QRect(cx - fillW, r.top(), fillW, r.height());
      fillColor = QColor(0xe6, 0xa8, 0x17);
    }
    p.setBrush(fillColor);
    p.drawRect(fillRect);
  }

  // Centre divider
  p.setPen(QPen(QColor(0x66, 0x66, 0x66), 1));
  p.drawLine(cx, r.top(), cx, r.bottom());

  // Outer border
  p.setPen(QPen(QColor(0x55, 0x55, 0x55), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(r, 3, 3);

  // Text
  p.setPen(QColor(0xee, 0xee, 0xee));
  QString label;
  if (_maxMmPerSec > 0.0) {
    label = QString("%1%  /  max %2 mm/s")
                .arg(_percent, 0, 'f', 0)
                .arg(_maxMmPerSec, 0, 'f', 0);
  } else {
    label = QString("—");
  }
  p.drawText(r, Qt::AlignCenter, label);
}
