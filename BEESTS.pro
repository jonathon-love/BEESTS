
QT       += core gui webkit webkitwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BEESTS
TEMPLATE -= app

unix:LIBS += -L/usr/lib/x86_64-linux-gnu/mesa/

windows:RC_FILE = ui/icon.rc
unix:ICON = ui/app.icns

SOURCES += ui/main.cpp\
    ui/mainwindow.cpp \
    ui/datasettablemodel.cpp \
    ui/dataset.cpp \
    ui/csv.cpp

HEADERS  += ui/mainwindow.h \
    ui/datasettablemodel.h \
    ui/dataset.h \
    ui/csv.h

FORMS += ui/mainwindow.ui

RESOURCES +=

unix:OTHER_FILES += ui/app.icns

windows:OTHER_FILES += \
    ui/icon.rc
