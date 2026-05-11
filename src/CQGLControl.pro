APPNAME = CQGLControl

include($$(MAKE_DIR)/qt_lib.mk)

QT += opengl

HEADERS += \
../../include/CQGLControl/CQGLControl.h

SOURCES += \
CQGLControl.cpp

INCLUDEPATH += \
$(INC_DIR)/CGLUtil \
$(INC_DIR)/CQIconButton \
$(INC_DIR)/CQPixmapCache \
$(INC_DIR)/CImageLib \
$(INC_DIR)/CFont \
$(INC_DIR)/CMath \
$(INC_DIR)/CStrUtil \
$(INC_DIR)/CFile \
$(INC_DIR)/CFileType \
$(INC_DIR)/CRefPtr \
$(INC_DIR)/CAlignType \
$(INC_DIR)/CRGBA \
$(INC_DIR)/COptVal \
$(INC_DIR)/CFlags \
$(INC_DIR)/COS \
