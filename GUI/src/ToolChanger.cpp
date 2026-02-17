#include "ToolChanger.hpp"

#include "StyledPopup.hpp"
#include "magic_enum/magic_enum.hpp"
#include "spdlog/spdlog.h"
#include "ui_ToolChanger.h"

using namespace utl;

ToolChanger::ToolChanger(QWidget* parent)
    : QWidget(parent), _ui(new Ui::ToolChanger) {
  _ui->setupUi(this);

  connect(_ui->closeButton, &QPushButton::clicked, this,
          &ToolChanger::handleButtons);
  connect(_ui->openButon, &QPushButton::clicked, this,
          &ToolChanger::handleButtons);
}
void ToolChanger::setArm(EArm arm) { this->_arm = arm; }
void ToolChanger::applyViewModel(const ToolChangerViewModel& vm) const {
  _ui->proxLamp->setState(vm.prox);
  _ui->openLamp->setState(vm.openSensor);
  _ui->closedLamp->setState(vm.closedSensor);
  _ui->valveOpenLamp->setState(vm.openValve);
  _ui->valveClosedLamp->setState(vm.closedValve);
  _ui->closeButton->setEnabled(vm.closeButtonEnabled);
  _ui->openButon->setEnabled(vm.openButtonEnabled);
}
void ToolChanger::handleButtons() {
  const auto sender = QObject::sender();
  GuiCommand command;
  auto reply = QMessageBox::No;
  if (sender == _ui->closeButton) {
    command.payload =
        GuiToolChangerCommand{.arm = _arm, .action = EToolChangerAction::Close};
    auto msg =
        std::format("Are you sure you want to close the {} tool changer?",
                    magic_enum::enum_name(_arm));
    if (*(_ui->proxLamp))
      reply = StyledPopup::askQuestion("Confirmation", QString::fromStdString(msg),
                                       QMessageBox::Yes | QMessageBox::No,
                                       QMessageBox::No, this);
    else {
      msg +=
          " Proximity sensor did not detect the clamp-side tool changer. Is it "
          "intentional?";
      reply = StyledPopup::askWarning("Warning", QString::fromStdString(msg),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No, this);
    }
  } else if (sender == _ui->openButon) {
    command.payload =
        GuiToolChangerCommand{.arm = _arm, .action = EToolChangerAction::Open};
    const auto msg =
        std::format("Are you sure you want to open the {} tool changer?",
                    magic_enum::enum_name(_arm));
    reply = StyledPopup::askQuestion("Confirmation", QString::fromStdString(msg),
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No, this);
  }
  if (reply == QMessageBox::Yes) emit buttonPressed(command);
}
