//
// Created by piotrek on 6/6/25.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "CommonDefinitions.hpp"
#include "MotorStats.hpp"

#include "RimoClient.h"

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

 private slots:
  static void on_pushButton_clicked();

 private:
  utl::RimoClient client;
  Ui::MainWindow *ui;
  std::map<utl::EMotor, MotorStats *> motorStats;
};

#endif  // MAINWINDOW_H
