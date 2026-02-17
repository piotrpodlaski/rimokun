#pragma once
#include <QPointer>
#include <qobject.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "CommonDefinitions.hpp"
#include "GuiCommand.hpp"
#include "GuiMetaTypes.hpp"
#include "ITransportWorker.hpp"

class Updater final : public QObject {
  Q_OBJECT
 public:
  using ErrorNotifier = std::function<void(const QString&, const QString&)>;

  explicit Updater(QObject* parent = nullptr,
                   std::unique_ptr<ITransportWorker> worker = nullptr,
                   ErrorNotifier errorNotifier = {});
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
  void handleResponse(const ITransportWorker::ResponseEvent& event);
  std::uint64_t nextRequestId();
  std::atomic<bool> _running{false};
  std::atomic<std::uint64_t> _requestCounter{1};
  std::mutex _pendingMutex;
  std::unordered_map<std::uint64_t, PendingRequestMeta> _pendingById;
  std::unique_ptr<ITransportWorker> _worker;
  ErrorNotifier _errorNotifier;
};
