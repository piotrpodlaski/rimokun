#include <QApplication>

#include "MainWindow.hpp"
int main(int argc, char* argv[]) {

  QApplication a(argc, argv);
  QApplication::setStyle("Fusion");
  MainWindow w;
  w.show();
  auto retCode =QApplication::exec();
  return retCode;
}