
#include <QApplication>

#include "utils/utils.h"
#include "mainwindow.h"

int qt_main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
static_block {
    register_main_function("qmmpp", qt_main);
}
