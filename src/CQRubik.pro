TEMPLATE = app

QT += widgets opengl

TARGET = CQRubik

DEPENDPATH += .

QMAKE_CXXFLAGS += -std=c++11

#CONFIG += debug

# Input
SOURCES += \
CQRubik.cpp \
\
CGLTexture.cpp \
CGLUtil.cpp \
CQGLControl.cpp \
CQWinWidget.cpp \

HEADERS += \
CQRubik.h \
\
CGLTexture.h \
CGLUtil.h \
CQGLControl.h \
CQWinWidget.h \

DESTDIR     = ../bin
OBJECTS_DIR = ../obj
LIB_DIR     = ../lib

INCLUDEPATH += \
../include \
../../CQUtil/include \
../../CImageLib/include \
../../CFont/include \
../../CUndo/include \
../../CFile/include \
../../CMath/include \
../../CStrUtil/include \
../../COS/include \
../../CUtil/include \
.

unix:LIBS += \
-L$$LIB_DIR \
-L../../CQUtil/lib \
-L../../CImageLib/lib \
-L../../CFont/lib \
-L../../CConfig/lib \
-L../../CUndo/lib \
-L../../CFile/lib \
-L../../CFileUtil/lib \
-L../../CStrUtil/lib \
-L../../CRegExp/lib \
-L../../COS/lib \
-L../../CUtil/lib \
-lCQUtil -lCImageLib -lCFont -lCConfig \
-lCUndo -lCFile -lCFileUtil -lCStrUtil -lCRegExp -lCOS -lCUtil \
-lglut -lGLU -lGL -lpng -ljpeg -ltre
