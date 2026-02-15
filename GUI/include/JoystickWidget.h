#pragma once

#include <QWidget>

class JoystickWidget : public QWidget {
 public:
  explicit JoystickWidget(QWidget* parent = nullptr);
  void setXy(double x, double y);
  void setButton(bool pressed);

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  static double clampUnit(double value);

  double _x{0.0};
  double _y{0.0};
  bool _pressed{false};
};
