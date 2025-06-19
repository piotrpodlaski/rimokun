#pragma once

#include "CommonDefinitions.hpp"
#include "QWidget"
#include "yaml-cpp/node/node.h"

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
  void handleButtons() const;

 signals:
  void buttonPressed(YAML::Node& button) const;

 private:
  Ui::ToolChanger* ui;
  utl::EArm arm;
};