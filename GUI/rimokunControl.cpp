#include <QApplication>
#include <QPushButton>
#include "MainWindow.hpp"
int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return QApplication::exec();
}