#pragma once
#include <QMetaType>
#include <QPointer>
#include <qobject.h>

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>

#include "CommonDefinitions.hpp"
#include "RimoClient.hpp"

Q_DECLARE_METATYPE(utl::RobotStatus);

class Updater final : public QObject {
  Q_OBJECT
 public:
  explicit Updater(QObject* parent = nullptr);
  ~Updater() override;
  void startUpdaterThread();
  void stopUpdaterThread();

 signals:
  void newDataArrived(const utl::RobotStatus& staus);
  void serverNotConnected();

 public slots:
  void sendCommand(const YAML::Node& command);

 private:
  struct PendingCommand {
    YAML::Node command;
    QPointer<QObject> sender;
    bool expectsResponse{false};
    QString senderClassName;
  };

  void runner();
  void processPendingCommands();
  std::atomic<bool> _running{false};
  std::mutex _commandMutex;
  std::queue<PendingCommand> _pendingCommands;

  utl::RimoClient<utl::RobotStatus> _client;
  std::thread _updaterThread;
};
