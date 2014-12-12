
QT       += core gui webkit webkitwidgets

#CONFIG += c++11
#CONFIG   -= app_bundle

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BEESTS
TEMPLATE -= app

unix:LIBS += -L/usr/lib/x86_64-linux-gnu/mesa/

windows:INCLUDEPATH += C:/progra~1/boost/boost_1_53_0
unix:INCLUDEPATH += /opt/local/include

windows:RC_FILE = ui/icon.rc
unix:ICON = ui/app.icns

SOURCES += ui/main.cpp\
		ui/mainwindow.cpp \
	ui/datasettablemodel.cpp \
	ui/csvparser.cpp \
	ui/dataset.cpp

HEADERS  += ui/mainwindow.h \
	ui/datasettablemodel.h \
	ui/csvparser.h \
	ui/dataset.h

FORMS    += ui/mainwindow.ui

RESOURCES += \
    ui/resources.qrc

unix:OTHER_FILES += ui/app.icns

windows:OTHER_FILES += \
	ui/icon.rc

OTHER_FILES += \
    ui/info.html
