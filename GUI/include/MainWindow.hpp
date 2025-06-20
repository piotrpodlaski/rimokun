//
// Created by piotrek on 6/6/25.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "CommonDefinitions.hpp"
#include "MotorStats.hpp"
#include "Updater.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "QtLogSink.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

 private:
  Updater updater;
  Ui::MainWindow *ui;
  utl::MotorStatsMap_t motorStats;
  std::shared_ptr<spdlog::logger> logger;
  std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> consoleSink;
  std::shared_ptr<QtLogSink> qtSink;

};

#endif  // MAINWINDOW_H
