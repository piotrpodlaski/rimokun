#pragma once

#include <QDialog>
#include <array>

class JoystickWidget;
class QCloseEvent;

class JoystickPanelWindow : public QDialog {
 public:
  explicit JoystickPanelWindow(QWidget* parent = nullptr);
  void setState(int id, double x, double y, bool pressed);

 protected:
  void closeEvent(QCloseEvent* event) override;

 private:
  std::array<JoystickWidget*, 3> _widgets{};
};
