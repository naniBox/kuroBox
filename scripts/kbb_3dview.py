#!/usr/bin/env python2
"""
	kuroBox / naniBox
	Copyright (c) 2013
	david morris-oliveros // naniBox.com

	This file is part of kuroBox / naniBox.

	kuroBox / naniBox is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	kuroBox / naniBox is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""

from OpenGL.GL import *
from OpenGL.GLU import *
from PyQt4 import QtGui
from PyQt4.QtOpenGL import *
import math

class KBB_3D_View(QGLWidget):
	def __init__(self, parent):
		QGLWidget.__init__(self, parent)
		self.yaw = 0.0
		self.pitch = 0.0
		self.roll = 0.0
		self.mkCam()

	def drawCube(self):
		glBegin(GL_QUADS)            
		glColor3f(0.0,1.0,0.0)            
		glVertex3f( 1.0, 1.0,-1.0)        
		glVertex3f(-1.0, 1.0,-1.0)        
		glVertex3f(-1.0, 1.0, 1.0)        
		glVertex3f( 1.0, 1.0, 1.0)        

		glColor3f(1.0,0.5,0.0)            
		glVertex3f( 1.0,-1.0, 1.0)        
		glVertex3f(-1.0,-1.0, 1.0)        
		glVertex3f(-1.0,-1.0,-1.0)        
		glVertex3f( 1.0,-1.0,-1.0)        

		glColor3f(1.0,0.0,0.0)            
		glVertex3f( 1.0, 1.0, 1.0)        
		glVertex3f(-1.0, 1.0, 1.0)        
		glVertex3f(-1.0,-1.0, 1.0)        
		glVertex3f( 1.0,-1.0, 1.0)        

		glColor3f(1.0,1.0,0.0)            
		glVertex3f( 1.0,-1.0,-1.0)        
		glVertex3f(-1.0,-1.0,-1.0)        
		glVertex3f(-1.0, 1.0,-1.0)        
		glVertex3f( 1.0, 1.0,-1.0)        

		glColor3f(0.0,0.0,1.0)            
		glVertex3f(-1.0, 1.0, 1.0)        
		glVertex3f(-1.0, 1.0,-1.0)        
		glVertex3f(-1.0,-1.0,-1.0)        
		glVertex3f(-1.0,-1.0, 1.0)        

		glColor3f(1.0,0.0,1.0)            
		glVertex3f( 1.0, 1.0,-1.0)        
		glVertex3f( 1.0, 1.0, 1.0)        
		glVertex3f( 1.0,-1.0, 1.0)        
		glVertex3f( 1.0,-1.0,-1.0)        
		glEnd()                

	def mkCam(self):
		w = 1.0/2.0
		l = 2.0/2.0
		h = 1.5/2.0
		
		ca = []
		# back
		ca.append([  w,  h, -l])
		ca.append([  w, -h, -l])
		ca.append([  w, -h, -l])
		ca.append([ -w, -h, -l])
		ca.append([ -w, -h, -l])
		ca.append([ -w,  h, -l])
		ca.append([ -w,  h, -l])
		ca.append([  w,  h, -l])

		# front
		ca.append([  w,  h,  l])
		ca.append([  w, -h,  l])
		ca.append([  w, -h,  l])
		ca.append([ -w, -h,  l])
		ca.append([ -w, -h,  l])
		ca.append([ -w,  h,  l])
		ca.append([ -w,  h,  l])
		ca.append([  w,  h,  l])

		# sides
		ca.append([  w,  h,  l])
		ca.append([  w,  h, -l])
		ca.append([  w, -h,  l])
		ca.append([  w, -h, -l])
		ca.append([ -w, -h,  l])
		ca.append([ -w, -h, -l])
		ca.append([ -w,  h,  l])
		ca.append([ -w,  h, -l])

		# "lens" front
		ca.append([  w*1.5,  h*1.5,  l*2.0])
		ca.append([  w*1.5, -h*1.5,  l*2.0])
		ca.append([  w*1.5, -h*1.5,  l*2.0])
		ca.append([ -w*1.5, -h*1.5,  l*2.0])
		ca.append([ -w*1.5, -h*1.5,  l*2.0])
		ca.append([ -w*1.5,  h*1.5,  l*2.0])
		ca.append([ -w*1.5,  h*1.5,  l*2.0])
		ca.append([  w*1.5,  h*1.5,  l*2.0])

		# "lens" sides
		ca.append([  w*1.5,  h*1.5,  l*2.0])
		ca.append([  w,      h*0.8,  l])
		ca.append([  w*1.5, -h*1.5,  l*2.0])
		ca.append([  w,     -h*0.8,  l])
		ca.append([ -w*1.5, -h*1.5,  l*2.0])
		ca.append([ -w,     -h*0.8,  l])
		ca.append([ -w*1.5,  h*1.5,  l*2.0])
		ca.append([ -w,      h*0.8,  l])

		self.cam_array = ca

	def drawCam(self):
		glColor(0.0, 1.0, 0.0)
		glEnableClientState(GL_VERTEX_ARRAY)
		glVertexPointerf(self.cam_array)
		glDrawArrays(GL_LINES, 0, len(self.cam_array))

	def drawStrange(self):
		self.spiral_array = []
		radius = 2.0
		x = radius*math.sin(0)
		y = radius*math.cos(0)
		z = radius*math.cos(0)*0.5
		for deg in xrange(820):
			self.spiral_array.append([x, y, z])
			rad = math.radians(deg)
			radius -= 0.001
			x = radius*math.sin(rad)
			y = radius*math.cos(rad)
			z = radius*math.cos(rad*5)*0.5

		glColor(0.0, 0.0, 1.0)
		glEnableClientState(GL_VERTEX_ARRAY)
		glVertexPointerf(self.spiral_array)
		glDrawArrays(GL_LINES, 0, len(self.spiral_array))


	def paintGL(self):
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

		glLoadIdentity()
		glTranslatef(0.0,0.0,-10.0)
		glRotatef(self.roll, 	0.0, 0.0, -1.0);
		glRotatef(self.pitch, 	1.0, 0.0,  0.0);
		glRotatef(self.yaw, 	0.0, 1.0,  0.0);
		#self.drawCube()
		self.drawCam()


	def setYPR(self, yaw, pitch, roll):
		self.yaw = yaw
		self.pitch = pitch
		self.roll = roll
		self.updateGL()

	def resizeGL(self, w, h):
		'''
		Resize the GL window 
		'''
		
		glViewport(0, 0, w, h)        # Reset The Current Viewport And Perspective Transformation
		glMatrixMode(GL_PROJECTION)
		glLoadIdentity()
		gluPerspective(45.0, float(w)/float(h), 0.1, 100.0)
		glMatrixMode(GL_MODELVIEW)

	def initializeGL(self):
		'''
		Initialize GL
		'''
		
		glClearColor(0.0, 0.0, 0.0, 0.0)    # This Will Clear The Background Color To Black
		glClearDepth(1.0)                    # Enables Clearing Of The Depth Buffer
		glDepthFunc(GL_LESS)                # The Type Of Depth Test To Do
		glEnable(GL_DEPTH_TEST)                # Enables Depth Testing
		glShadeModel(GL_SMOOTH)                # Enables Smooth Color Shading

		glMatrixMode(GL_PROJECTION)
		glLoadIdentity()