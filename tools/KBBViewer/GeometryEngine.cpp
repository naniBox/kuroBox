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
#include "GeometryEngine.h"

#include <QVector2D>
#include <QVector3D>

//-----------------------------------------------------------------------------
struct VertexData
{
    QVector3D position;
    QVector2D texCoord;
};

//-----------------------------------------------------------------------------
GeometryEngine::GeometryEngine()
{
}

//-----------------------------------------------------------------------------
GeometryEngine::~GeometryEngine()
{
	glDeleteBuffers(4, vboIds);
}

//-----------------------------------------------------------------------------
void GeometryEngine::init()
{
    initializeGLFunctions();
	glGenBuffers(4, vboIds);
	initGeometry();
}

//-----------------------------------------------------------------------------
void GeometryEngine::initGeometry()
{
    // For cube we would need only 8 vertices but we have to
    // duplicate vertex for each face because texture coordinate
    // is different.
    VertexData vertices[] = {
        // Vertex data for face 0
		{QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.0f,  0.0f)}, // v0
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D(0.33f, 0.0f)}, // v1
		{QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(0.0f,  0.5f)}, // v2
        {QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(0.33f, 0.5f)}, // v3

        // Vertex data for face 1
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D( 0.0f, 0.5f)}, // v4
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(0.33f, 0.5f)}, // v5
		{QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(0.0f,  1.0f)}, // v6
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(0.33f, 1.0f)}, // v7

        // Vertex data for face 2
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(0.66f, 0.5f)}, // v8
		{QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(1.0f,  0.5f)}, // v9
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(0.66f, 1.0f)}, // v10
		{QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(1.0f,  1.0f)}, // v11

        // Vertex data for face 3
        {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(0.66f, 0.0f)}, // v12
		{QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(1.0f,  0.0f)}, // v13
        {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.66f, 0.5f)}, // v14
		{QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(1.0f,  0.5f)}, // v15

        // Vertex data for face 4
        {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(0.33f, 0.0f)}, // v16
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(0.66f, 0.0f)}, // v17
        {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.33f, 0.5f)}, // v18
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D(0.66f, 0.5f)}, // v19

        // Vertex data for face 5
        {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(0.33f, 0.5f)}, // v20
        {QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(0.66f, 0.5f)}, // v21
        {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.33f, 1.0f)}, // v22
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(0.66f, 1.0f)}  // v23
    };

    // Indices for drawing cube faces using triangle strips.
    // Triangle strips can be connected by duplicating indices
    // between the strips. If connecting strips have opposite
    // vertex order then last index of the first strip and first
    // index of the second strip needs to be duplicated. If
    // connecting strips have same vertex order then only last
    // index of the first strip needs to be duplicated.
    GLushort indices[] = {
         0,  1,  2,  3,  3,     // Face 0 - triangle strip ( v0,  v1,  v2,  v3)
         4,  4,  5,  6,  7,  7, // Face 1 - triangle strip ( v4,  v5,  v6,  v7)
         8,  8,  9, 10, 11, 11, // Face 2 - triangle strip ( v8,  v9, v10, v11)
        12, 12, 13, 14, 15, 15, // Face 3 - triangle strip (v12, v13, v14, v15)
        16, 16, 17, 18, 19, 19, // Face 4 - triangle strip (v16, v17, v18, v19)
        20, 20, 21, 22, 23      // Face 5 - triangle strip (v20, v21, v22, v23)
    };

    // Transfer vertex data to VBO 0
    glBindBuffer(GL_ARRAY_BUFFER, vboIds[0]);
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(VertexData), vertices, GL_STATIC_DRAW);

    // Transfer index data to VBO 1
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIds[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 34 * sizeof(GLushort), indices, GL_STATIC_DRAW);

	// grid here
	QVector3D gridVertex[10*2];
	GLushort gridIdx[10*2];
	for ( int i = 0 ; i < 10 ; i++ )
	{
		gridVertex[i*2+0] = QVector3D(-10.0f, 1.0f, i);
		gridVertex[i*2+1] = QVector3D( 10.0f,-1.0f, i);
		gridIdx[i*2+0] = i*2+0;
		gridIdx[i*2+1] = i*2+1;
	}
	glBindBuffer(GL_ARRAY_BUFFER, vboIds[2]);
	glBufferData(GL_ARRAY_BUFFER, 10*2 * sizeof(QVector3D), gridVertex, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIds[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 10*2 * sizeof(GLushort), gridIdx, GL_STATIC_DRAW);
}

//-----------------------------------------------------------------------------
void GeometryEngine::drawCubeGeometry(QGLShaderProgram *program)
{
    // Tell OpenGL which VBOs to use
    glBindBuffer(GL_ARRAY_BUFFER, vboIds[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIds[1]);

    // Offset for position
    quintptr offset = 0;

    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("a_position");
    program->enableAttributeArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (const void *)offset);

    // Offset for texture coordinate
    offset += sizeof(QVector3D);

    // Tell OpenGL programmable pipeline how to locate vertex texture coordinate data
    int texcoordLocation = program->attributeLocation("a_texcoord");
    program->enableAttributeArray(texcoordLocation);
    glVertexAttribPointer(texcoordLocation, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (const void *)offset);

    // Draw cube geometry using indices from VBO 1
	glDrawElements(GL_TRIANGLE_STRIP, 34, GL_UNSIGNED_SHORT, 0);
}

//-----------------------------------------------------------------------------
void GeometryEngine::drawGridGeometry(QGLShaderProgram *program)
{
	/*
	glBindBuffer(GL_ARRAY_BUFFER, vboIds[2]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIds[3]);

	int vertexLocation = program->attributeLocation("a_position");
	program->enableAttributeArray(vertexLocation);
	glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (const void *)0);

	glDrawElements(GL_LINES, 10*2, GL_UNSIGNED_SHORT, 0);

	*/

	program->setUniformValue("a_colour", QVector4D(0.3f, 0.3f, 0.3f, 0.5f));
	glBegin(GL_LINES);
	const int grid_size = 20;
	for ( int i = 3 ; i <= grid_size ; i++ )
	{
		glVertex3f((float)-grid_size, 0.0f, (float)i);
		glVertex3f((float) grid_size, 0.0f, (float)i);
		glVertex3f((float)i, 0.0f, (float)-grid_size);
		glVertex3f((float)i, 0.0f, (float) grid_size);

		glVertex3f((float)-grid_size, 0.0f, (float)-i);
		glVertex3f((float) grid_size, 0.0f, (float)-i);
		glVertex3f((float)-i, 0.0f, (float)-grid_size);
		glVertex3f((float)-i, 0.0f, (float) grid_size);
	}
	for ( int i = -2 ; i <= 2 ; i++ )
	{
		glVertex3f((float)i, 0.0f, 3.0f);
		glVertex3f((float)i, 0.0f, (float)grid_size);

		glVertex3f((float)i, 0.0f, -3.0f);
		glVertex3f((float)i, 0.0f, -(float)grid_size);

		glVertex3f( 3.0f,             0.0f, (float)i);
		glVertex3f( (float)grid_size, 0.0f, (float)i);

		glVertex3f(-3.0f,             0.0f, (float)i);
		glVertex3f(-(float)grid_size, 0.0f, (float)i);
	}
	glEnd();

	program->setUniformValue("a_colour", QVector4D(0.3f, 0.3f, 0.75f, 0.75f));
	glBegin(GL_LINE_STRIP);
	glVertex3f( 0.0f, 0.0f,  3.0f);
	glVertex3f( 3.0f, 0.0f,  0.0f);
	glVertex3f( 1.0f, 0.0f,  0.0f);
	glVertex3f( 1.0f, 0.0f, -3.0f);
	glVertex3f(-1.0f, 0.0f, -3.0f);
	glVertex3f(-1.0f, 0.0f,  0.0f);
	glVertex3f(-3.0f, 0.0f,  0.0f);
	glVertex3f( 0.0f, 0.0f,  3.0f);
	glEnd();
}

//-----------------------------------------------------------------------------
void GeometryEngine::drawCameraGeometry(QGLShaderProgram *program)
{
	program->setUniformValue("a_colour", QVector4D(0.0f, 1.0f, 0.2f, 1.0f));
	glBegin(GL_LINES);

		const float w = 1.0f/2.0f;
		const float l = 2.0f/2.0f;
		const float h = 1.5f/2.0f;

		// back
		glVertex3f(  w,  h, -l);
		glVertex3f(  w, -h, -l);
		glVertex3f(  w, -h, -l);
		glVertex3f( -w, -h, -l);
		glVertex3f( -w, -h, -l);
		glVertex3f( -w,  h, -l);
		glVertex3f( -w,  h, -l);
		glVertex3f(  w,  h, -l);

		// front
		glVertex3f(  w,  h,  l);
		glVertex3f(  w, -h,  l);
		glVertex3f(  w, -h,  l);
		glVertex3f( -w, -h,  l);
		glVertex3f( -w, -h,  l);
		glVertex3f( -w,  h,  l);
		glVertex3f( -w,  h,  l);
		glVertex3f(  w,  h,  l);

		// sides
		glVertex3f(  w,  h,  l);
		glVertex3f(  w,  h, -l);
		glVertex3f(  w, -h,  l);
		glVertex3f(  w, -h, -l);
		glVertex3f( -w, -h,  l);
		glVertex3f( -w, -h, -l);
		glVertex3f( -w,  h,  l);
		glVertex3f( -w,  h, -l);

		// "lens" front
		glVertex3f(  w*3.5f,  h*1.5f,  l*2.0f);
		glVertex3f(  w*3.5f, -h*1.5f,  l*2.0f);
		glVertex3f(  w*3.5f, -h*1.5f,  l*2.0f);
		glVertex3f( -w*3.5f, -h*1.5f,  l*2.0f);
		glVertex3f( -w*3.5f, -h*1.5f,  l*2.0f);
		glVertex3f( -w*3.5f,  h*1.5f,  l*2.0f);
		glVertex3f( -w*3.5f,  h*1.5f,  l*2.0f);
		glVertex3f(  w*3.5f,  h*1.5f,  l*2.0f);

		// "lens" sides
		glVertex3f(  w*3.5f,  h*1.5f,  l*2.0f);
		glVertex3f(  w,       h*0.8f,  l);
		glVertex3f(  w*3.5f, -h*1.5f,  l*2.0f);
		glVertex3f(  w,      -h*0.8f,  l);
		glVertex3f( -w*3.5f, -h*1.5f,  l*2.0f);
		glVertex3f( -w,      -h*0.8f,  l);
		glVertex3f( -w*3.5f,  h*1.5f,  l*2.0f);
		glVertex3f( -w,       h*0.8f,  l);
	glEnd();

	program->setUniformValue("a_colour", QVector4D(0.3f, 0.3f, 0.85f, 1.0f));
	glBegin(GL_LINE_STRIP);
	glVertex3f( 0.0f,	h,		-l);
	glVertex3f( w,		0.0f,	-l);
	glVertex3f( w/2.0f, 0.0f,	-l);
	glVertex3f( w/2.0f, -h,		-l);
	glVertex3f(-w/2.0f, -h,		-l);
	glVertex3f(-w/2.0f, 0.0f,	-l);
	glVertex3f(-w,		0.0f,	-l);
	glVertex3f( 0.0f,	h,		-l);
	glEnd();
}
