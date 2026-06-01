#include "HomingWindow.hpp"

#include <QGridLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
QPushButton* makeHomingButton(const QString& text, QWidget* parent) {
  auto* button = new QPushButton(text, parent);
  button->setMinimumHeight(42);
  return button;
}
}  // namespace

HomingWindow::HomingWindow(QWidget* parent) : QDialog(parent) {
  setWindowTitle("Homing");
  setModal(false);

  auto* root = new QVBoxLayout(this);
  auto* grid = new QGridLayout();
  grid->setSpacing(8);
  root->addLayout(grid);

  auto* all = makeHomingButton("Home all", this);
  auto* xLeft = makeHomingButton("Home X left", this);
  auto* yLeft = makeHomingButton("Home Y left", this);
  auto* xRight = makeHomingButton("Home X right", this);
  auto* yRight = makeHomingButton("Home Y right", this);
  auto* z = makeHomingButton("Home Z axis", this);

  grid->addWidget(all, 0, 0, 1, 2);
  grid->addWidget(xLeft, 1, 0);
  grid->addWidget(yLeft, 2, 0);
  grid->addWidget(xRight, 1, 1);
  grid->addWidget(yRight, 2, 1);
  grid->addWidget(z, 3, 0, 1, 2);

  connect(all, &QPushButton::clicked, this,
          [this] { emitHomingCommand(cmd::HomingTarget::All); });
  connect(xLeft, &QPushButton::clicked, this,
          [this] { emitHomingCommand(cmd::HomingTarget::XLeft); });
  connect(yLeft, &QPushButton::clicked, this,
          [this] { emitHomingCommand(cmd::HomingTarget::YLeft); });
  connect(xRight, &QPushButton::clicked, this,
          [this] { emitHomingCommand(cmd::HomingTarget::XRight); });
  connect(yRight, &QPushButton::clicked, this,
          [this] { emitHomingCommand(cmd::HomingTarget::YRight); });
  connect(z, &QPushButton::clicked, this,
          [this] { emitHomingCommand(cmd::HomingTarget::Z); });
}

void HomingWindow::emitHomingCommand(const cmd::HomingTarget target) const {
  GuiCommand command;
  command.payload = cmd::HomingCommand{.target = target};
  emit commandIssued(command);
}
