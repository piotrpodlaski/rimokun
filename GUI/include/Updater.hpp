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

 private:
  void runner();
  bool running{false};

  utl::RimoClient client;
  std::thread updaterThread;
};