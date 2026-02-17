#pragma once

#include <QPushButton>
#include <QWidget>
#include <GuiCommand.hpp>
#include <ResponseConsumer.hpp>

namespace Ui {
class AuxControls;
}

class AuxControls final : public QWidget, public ResponseConsumer {
  Q_OBJECT
 public:
  explicit AuxControls(QWidget* parent = nullptr);
  void processResponse(const GuiResponse& response) override;

 private slots:
  void handleButtons();

 signals:
  void buttonPressed(const GuiCommand& command) const;

 private:
  Ui::AuxControls* _ui;
  QPushButton* _bEnable;
  QPushButton* _bDisable;
  QPushButton* _bLoad;
  QPushButton* _bSave;
};
