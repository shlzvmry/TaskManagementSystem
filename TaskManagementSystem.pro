QT       += core gui sql printsupport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \

HEADERS += \
    mainwindow.h \

#数据库模块
SOURCES += \
    database/database.cpp\

HEADERS += \
    database/database.h\

#模型模块
SOURCES += \
    models/taskmodel.cpp \
    models/inspirationmodel.cpp \
    models/taskfiltermodel.cpp \
    models/taskitem.cpp \
    models/statisticmodel.cpp

HEADERS += \
    models/taskmodel.h \
    models/inspirationmodel.h \
    models/taskfiltermodel.h \
    models/taskitem.h\
    models/statisticmodel.h

#视图模块
SOURCES += \
    views/kanbanview.cpp \
    views/calenderview.cpp \
    views/tasktableview.cpp \
    views/inspirationview.cpp\
    views/statisticview.cpp\

HEADERS += \
    views/kanbanview.h \
    views/calenderview.h \
    views/tasktableview.h \
    views/inspirationview.h\
    views/statisticview.h\

#对话框模块
SOURCES += \
    dialogs/taskdialog.cpp\
    dialogs/recyclebindialog.cpp\
    dialogs/inspirationdialog.cpp \
    dialogs/inspirationrecyclebindialog.cpp \
    dialogs/tagmanagerdialog.cpp \
    dialogs/inspirationtagsearchdialog.cpp\

HEADERS += \
    dialogs/taskdialog.h \
    dialogs/recyclebindialog.h\
    dialogs/inspirationdialog.h \
    dialogs/inspirationrecyclebindialog.h \
    dialogs/tagmanagerdialog.h \
    dialogs/inspirationtagsearchdialog.h\

FORMS += \
    dialogs/inspirationdialog.ui \
    dialogs/recyclebindialog.ui \
    dialogs/taskdialog.ui \
    dialogs/tagmanagerdialog.ui \

#控件模块
SOURCES += \
    widgets/watermarkwidget.cpp \
    widgets/tagwidget.cpp \
    widgets/prioritywidget.cpp \
    widgets/statuswidget.cpp \
    widgets/comboboxdelegate.cpp\
    widgets/simplechartwidget.cpp \

HEADERS += \
    widgets/watermarkwidget.h \
    widgets/tagwidget.h \
    widgets/prioritywidget.h \
    widgets/statuswidget.h \
    widgets/comboboxdelegate.h\
    widgets/simplechartwidget.h \

#工具模块
SOURCES += \
    utils/exporter.cpp \

HEADERS += \
   utils/exporter.h \

# 包含路径
INCLUDEPATH += \
    $$PWD/database \
    $$PWD/widgets \
    $$PWD/models \
    $$PWD/dialogs \
    $$PWD/views \
    $$PWD/utils\

# Default rules for deployment.
CODECFORTR = UTF-8
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources/resources.qrc \
