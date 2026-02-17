#pragma once

#include <optional>

#include <QObject>

#include "CommonDefinitions.hpp"
#include "GuiMetaTypes.hpp"

class GuiStateStore final : public QObject {
  Q_OBJECT
 public:
  explicit GuiStateStore(QObject* parent = nullptr);

  [[nodiscard]] std::optional<utl::RobotStatus> latestStatus() const;
  [[nodiscard]] bool isConnected() const;

 public slots:
  void onStatusReceived(const utl::RobotStatus& status);
  void onServerDisconnected();

 signals:
  void statusUpdated(const utl::RobotStatus& status);
  void connectionChanged(bool connected);

 private:
  std::optional<utl::RobotStatus> _latestStatus;
  bool _connected{false};
};
