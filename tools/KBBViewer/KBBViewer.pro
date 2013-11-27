#-------------------------------------------------
#
# Project created by QtCreator 2013-11-24T00:28:49
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = KBBViewer
TEMPLATE = app


SOURCES += main.cpp\
	kbbviewer.cpp \
	KBB/KBB.cpp \
	KBB/utils.cpp \
    ThreeDWidget.cpp \
    GeometryEngine.cpp

HEADERS  += kbbviewer.h \
    KBB/KBB_types.h \
    KBB/KBB.h \
    KBB/utils.h \
    ThreeDWidget.h \
    GeometryEngine.h

FORMS    += kbbviewer.ui

DEFINES += KBB_QT=1

RESOURCES += \
    textures.qrc \
    shaders.qrc

OTHER_FILES += \
    shader_tex_f.glsl \
    shader_lines_f.glsl \
    shader_tex_v.glsl \
    shader_lines_v.glsl
