#include "mainwindow.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/tray_icon.png"));
    a.setApplicationName("个人工作与任务管理系统");
    a.setApplicationVersion("1.0.0");
    a.setOrganizationName("谢静蕾");

    MainWindow w;
    w.show();

    return a.exec();
}
