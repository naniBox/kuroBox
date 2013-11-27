/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

//-----------------------------------------------------------------------------
#include "ThreeDWidget.h"

#include <QMouseEvent>

#include <math.h>
#include <locale.h>

//-----------------------------------------------------------------------------
ThreeDWidget::ThreeDWidget(QWidget *parent) :
    QGLWidget(parent),
	rotation(0,0,0)
{
}

//-----------------------------------------------------------------------------
ThreeDWidget::~ThreeDWidget()
{
    deleteTexture(texture);
}

//-----------------------------------------------------------------------------
void ThreeDWidget::setYPR(float y, float p, float r)
{
	ypr = QVector3D(y,p,r);
	updateGL();
}

//-----------------------------------------------------------------------------
void ThreeDWidget::resetView()
{
	rotation = QVector3D(0,0,0);
	updateGL();
}

//-----------------------------------------------------------------------------
void ThreeDWidget::mousePressEvent(QMouseEvent * e)
{
    // Save mouse press position
	mousePressPosition = e->pos();
}

//-----------------------------------------------------------------------------
void ThreeDWidget::mouseMoveEvent(QMouseEvent * e)
{
	int dx = e->x() - mousePressPosition.x();
	int dy = e->y() - mousePressPosition.y();

	if (e->buttons() & Qt::LeftButton)
	{
		rotation.setX(rotation.x() + 8 * dy);
		rotation.setY(rotation.y() + 8 * dx);
	}
	else if (e->buttons() & Qt::RightButton)
	{
		rotation.setX(rotation.x() + 8 * dy);
		rotation.setZ(rotation.z() + 8 * dx);
	}
	mousePressPosition = e->pos();
	updateGL();
}

//-----------------------------------------------------------------------------
void ThreeDWidget::timerEvent(QTimerEvent * e)
{
	// updateGL();
}

//-----------------------------------------------------------------------------
void ThreeDWidget::initializeGL()
{
    initializeGLFunctions();
    qglClearColor(Qt::black);
    initShaders();
    initTextures();

    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);

    // Enable back face culling
    glEnable(GL_CULL_FACE);

    geometries.init();

    // Use QBasicTimer because its faster than QTimer
    timer.start(12, this);
}

//-----------------------------------------------------------------------------
void ThreeDWidget::initShaders()
{
	//if ( !initShader(programTex, ":/shader_tex_v.glsl", ":/shader_tex_f.glsl") )
	{
		// @TODO: BAD!
	}
	if ( !initShader(programLines, ":/shader_lines_v.glsl", ":/shader_lines_f.glsl") )
	{
		// @TODO: BAD!
	}

}

//-----------------------------------------------------------------------------
bool ThreeDWidget::initShader(QGLShaderProgram & program, const QString & vert, const QString & frag)
{
	// Override system locale until shaders are compiled
	setlocale(LC_NUMERIC, "C");

	bool ret = false;
	if (program.addShaderFromSourceFile(QGLShader::Vertex, vert))
		if (program.addShaderFromSourceFile(QGLShader::Fragment, frag))
			if (program.link())
				if (program.bind())
					ret = true;

	// Restore system locale
	setlocale(LC_ALL, "");
	return ret;

}

//-----------------------------------------------------------------------------
void ThreeDWidget::initTextures()
{
	/*
    // Load cube.png image
    glEnable(GL_TEXTURE_2D);
    texture = bindTexture(QImage(":/cube.png"));

    // Set nearest filtering mode for texture minification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Set bilinear filtering mode for texture magnification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Wrap texture coordinates by repeating
    // f.ex. texture coordinate (1.1, 1.2) is same as (0.1, 0.2)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	*/
}

//-----------------------------------------------------------------------------
void ThreeDWidget::resizeGL(int w, int h)
{
    // Set OpenGL viewport to cover whole widget
    glViewport(0, 0, w, h);

    // Calculate aspect ratio
    qreal aspect = qreal(w) / qreal(h ? h : 1);

    // Set near plane to 3.0, far plane to 7.0, field of view 45 degrees
	const qreal zNear = 0.1, zFar = 50.0, fov = 45.0;

    // Reset projection
    projection.setToIdentity();

    // Set perspective projection
    projection.perspective(fov, aspect, zNear, zFar);
}

//-----------------------------------------------------------------------------
void ThreeDWidget::paintGL()
{
    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Calculate model view transformation
    QMatrix4x4 matrix;
	matrix.translate(0.0, 0.0, -10.0);

	matrix.rotate(rotation.x() / 16.0f, 1.0f, 0.0f, 0.0f);
	matrix.rotate(rotation.y() / 16.0f, 0.0f, 1.0f, 0.0f);
	matrix.rotate(rotation.z() / 16.0f, 0.0f, 0.0f, 1.0f);

	glDisable(GL_TEXTURE_2D);
	programLines.bind();
	programLines.setUniformValue("mvp_matrix", projection * matrix);
	geometries.drawGridGeometry(&programLines);

	matrix.rotate(ypr.z(), 0.0f, 0.0f,-1.0f);
	matrix.rotate(ypr.y(), 1.0f, 0.0f, 0.0f);
	matrix.rotate(ypr.x(), 0.0f, 1.0f, 0.0f);

	glDisable(GL_TEXTURE_2D);
	programLines.bind();
	programLines.setUniformValue("mvp_matrix", projection * matrix);
	geometries.drawCameraGeometry(&programLines);
}
