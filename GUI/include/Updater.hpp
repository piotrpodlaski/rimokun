#pragma once
#include <qobject.h>
#include "CommonDefinitions.hpp"
#include <thread>

class Updater final : public QObject {
  Q_OBJECT
 public:
  explicit Updater(QObject* parent = nullptr);
  ~Updater() override;

  signals:
  void newDataArrived(utl::RobotStatus staus);

  private:
  std::thread updaterThread;
};