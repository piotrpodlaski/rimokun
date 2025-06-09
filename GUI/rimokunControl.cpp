#include <QApplication>

#include "MainWindow.hpp"
int main(int argc, char* argv[]) {
  QApplication a(argc, argv);
  QApplication::setStyle("Fusion");
  MainWindow w;
  w.show();
  return QApplication::exec();
}