QT       += core gui sql
#QT       += charts
#QT       += printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    models/taskitem.cpp

HEADERS += \
    mainwindow.h \
    models/taskitem.h\


#数据库模块
SOURCES += \
    database/database.cpp\

HEADERS += \
    database/database.h\

#模型模块
SOURCES += \
    models/taskmodel.cpp \
    models/inspirationmodel.cpp \

HEADERS += \
    models/taskmodel.h \
    models/inspirationmodel.h \

#对话框模块
SOURCES += \
    dialogs/taskdialog.cpp\

HEADERS += \
    dialogs/taskdialog.h \

FORMS += \
    dialogs/taskdialog.ui \
    dialogs/taskdialog.ui \

#控件模块
SOURCES += \
    widgets/watermarkwidget.cpp \
    widgets/tagwidget.cpp \
    widgets/prioritywidget.cpp \
    widgets/statuswidget.cpp \

HEADERS += \
    widgets/watermarkwidget.h \
    widgets/tagwidget.h \
    widgets/prioritywidget.h \
    widgets/statuswidget.h \

#样式表文件
DISTFILES += \
    styles/mainwindow.qss \
    styles/dialog.qss \
    styles/widget.qss

# 包含路径
INCLUDEPATH += \
    $$PWD/database \
    $$PWD/widgets \
    $$PWD/models \
    $$PWD/dialogs \

# Default rules for deployment.
CODECFORTR = UTF-8
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources/icons.qrc \
    resources/images.qrc\
