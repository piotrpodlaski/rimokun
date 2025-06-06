//
// Created by piotrek on 6/6/25.
//

// You may need to build the project (run Qt uic code generator) to get
// "ui_MainWindow.h" resolved

#include "MainWindow.hpp"

#include "ui_MainWindow.h"
#include <print>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  connect(ui->pushButton_2, &QPushButton::released, this, &MainWindow::on_pushButton_clicked);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_pushButton_clicked() {std::print("dupa\n");}
