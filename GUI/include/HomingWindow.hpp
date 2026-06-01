#pragma once

#include <QDialog>

#include "GuiCommand.hpp"

class HomingWindow final : public QDialog {
  Q_OBJECT
 public:
  explicit HomingWindow(QWidget* parent = nullptr);

 signals:
  void commandIssued(const GuiCommand& command) const;

 private:
  void emitHomingCommand(cmd::HomingTarget target) const;
};
