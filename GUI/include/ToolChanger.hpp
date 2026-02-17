#pragma once

#include "CommonDefinitions.hpp"
#include "GuiCommand.hpp"
#include "QWidget"

namespace Ui {
class ToolChanger;
}

class ToolChanger final : public QWidget {
  Q_OBJECT

 public:
  explicit ToolChanger(QWidget* parent = nullptr);
  ~ToolChanger() override = default;
  void setArm(utl::EArm arm);

 public slots:
  void updateRobotStatus(const utl::RobotStatus& robotStatus) const;

  private slots:
  void handleButtons();

 signals:
  void buttonPressed(const GuiCommand& command) const;

 private:
  Ui::ToolChanger* _ui;
  utl::EArm _arm;
};
