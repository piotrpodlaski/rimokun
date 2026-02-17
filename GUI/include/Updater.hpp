#pragma once
#include <QMetaType>
#include <QPointer>
#include <qobject.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <unordered_map>

#include "CommonDefinitions.hpp"
#include "GuiCommand.hpp"
#include "RimoTransportWorker.hpp"

Q_DECLARE_METATYPE(utl::RobotStatus);

class Updater final : public QObject {
  Q_OBJECT
 public:
  explicit Updater(QObject* parent = nullptr);
  ~Updater() override;
  void startUpdaterThread();
  void stopUpdaterThread();

 signals:
  void newDataArrived(const utl::RobotStatus& status);
  void serverNotConnected();

 public slots:
  void sendCommand(const GuiCommand& command);

 private:
  struct PendingRequestMeta {
    QPointer<QObject> sender;
    bool expectsResponse{false};
    QString senderClassName;
  };
  void handleResponse(const RimoTransportWorker::ResponseEvent& event);
  std::uint64_t nextRequestId();
  std::atomic<bool> _running{false};
  std::atomic<std::uint64_t> _requestCounter{1};
  std::mutex _pendingMutex;
  std::unordered_map<std::uint64_t, PendingRequestMeta> _pendingById;
  RimoTransportWorker _worker;
};
