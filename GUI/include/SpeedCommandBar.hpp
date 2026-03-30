#pragma once

#include <QWidget>

// Bidirectional speed command bar.
// Shows commanded speed as a percentage [-100, 100] of the mode maximum.
// The bar is centred at 0: positive values (forward) fill to the right in
// green, negative values (reverse) fill to the left in amber.
// The current percentage and the mode max speed (mm/s) are drawn as text
// inside the bar.
class SpeedCommandBar : public QWidget {
  Q_OBJECT

 public:
  explicit SpeedCommandBar(QWidget* parent = nullptr);

  // percent: [-100, 100]  – the joystick-commanded speed fraction
  // maxMmPerSec: mode maximum linear speed in mm/s (0 = unknown/locked)
  void setValues(double percent, double maxMmPerSec);

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  double _percent{0.0};
  double _maxMmPerSec{0.0};
};
