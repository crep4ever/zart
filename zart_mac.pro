TEMPLATE = app
QT += xml network
CONFIG	+= qt
#CONFIG	+= warn_on debug
INCLUDEPATH	+= .. ./include /opt/local/include /opt/local/include/opencv ../src/
DEPENDPATH += ./include

HEADERS	+= include/Settings.h \
           include/ImageView.h \
           include/MainWindow.h \
           include/WebcamGrabber.h \
           include/FilterThread.h \
           include/CommandEditor.h \
           include/DialogAbout.h \
           include/ImageConverter.h \
           include/DialogLicence.h

SOURCES	+= src/Settings.cpp \
           src/WebcamGrabber.cpp \
           src/ImageView.cpp \
           src/MainWindow.cpp \
           src/ZArt.cpp \
           src/FilterThread.cpp \
           src/DialogAbout.cpp \
           src/CommandEditor.cpp \
           src/ImageConverter.cpp \
           src/DialogLicence.cpp

RESOURCES = zart.qrc
FORMS = ui/MainWindow.ui ui/DialogAbout.ui ui/DialogLicence.ui

LIBS += -lX11 ../src/libgmic.a `pkg-config opencv --libs` -lfftw3
PRE_TARGETDEPS +=  
QMAKE_CXXFLAGS_DEBUG += -Dcimg_use_fftw3
QMAKE_CXXFLAGS_RELEASE += -ffast-math -Dcimg_use_fftw3
UI_DIR = .ui
MOC_DIR = .moc
OBJECTS_DIR = .obj

#unix: DEFINES += _IS_UNIX_

DEFINES += cimg_display=0

#QMAKE_LIBS =  
#QMAKE_LFLAGS_DEBUG = -lcxcore -lcv -lhighgui -lml  
#QMAKE_LFLAGS_RELEASE = -lcxcore -lcv -lhighgui -lml  
