#include <QApplication>
#include <logger.hpp>

#include "MainWindow.hpp"
int main(int argc, char* argv[]) {
  utl::configure_logger();
  QApplication a(argc, argv);
  QApplication::setStyle("Fusion");
  MainWindow w;
  w.show();
  const auto retCode = QApplication::exec();
  return retCode;
}