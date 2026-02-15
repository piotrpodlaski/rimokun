#include "JoystickPanelWindow.h"

#include "JoystickWidget.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

JoystickPanelWindow::JoystickPanelWindow(QWidget* parent) : QDialog(parent) {
  setWindowTitle("Control panel monitor");
  setWindowModality(Qt::NonModal);
  setWindowFlag(Qt::Tool, true);
  setAttribute(Qt::WA_DeleteOnClose, false);

  auto* root = new QHBoxLayout(this);
  root->setContentsMargins(10, 10, 10, 10);
  root->setSpacing(12);

  const std::array labels = {"Arm 1", "Arm 2", "Gantry"};
  for (int i = 0; i < 3; ++i) {
    auto* col = new QVBoxLayout();
    col->setSpacing(6);

    auto* title = new QLabel(labels[i], this);
    title->setAlignment(Qt::AlignCenter);
    col->addWidget(title);

    _widgets[i] = new JoystickWidget(this);
    col->addWidget(_widgets[i], 1);

    root->addLayout(col, 1);
  }
}

void JoystickPanelWindow::setState(int id, double x, double y, bool pressed) {
  if (id < 0 || id >= static_cast<int>(_widgets.size())) {
    return;
  }
  _widgets[static_cast<std::size_t>(id)]->setXy(x, y);
  _widgets[static_cast<std::size_t>(id)]->setButton(pressed);
}

void JoystickPanelWindow::closeEvent(QCloseEvent* event) {
  hide();
  event->ignore();
}
