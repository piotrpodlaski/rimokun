#pragma once
#include <qobject.h>

#include <thread>

#include "CommonDefinitions.hpp"
#include "RimoClient.hpp"

class Updater final : public QObject {
  Q_OBJECT
 public:
  explicit Updater(QObject* parent = nullptr);
  ~Updater() override;
  void startUpdaterThread();
  void stopUpdaterThread();

 signals:
  void newDataArrived(const utl::RobotStatus& staus);

 public slots:
  void sendCommand(const YAML::Node& command);

 private:
  void runner();
  bool _running{false};

  utl::RimoClient<utl::RobotStatus> _client;
  std::thread _updaterThread;
};