import sys
import os
from PyQt4 import QtCore, QtGui, uic
import KBB

def fmt_h32(x):		return "0x%08X"%x
def fmt_i(x):		return "%i"%x
def fmt_h8(x):		return "0x%02x"%x

class KBB_Viewer(QtGui.QMainWindow):
	def __init__(self):
		QtGui.QMainWindow.__init__(self)
		self.ui = uic.loadUi("kbb_viewer.ui", self)
		self.connect(self.ui.actionOpen, QtCore.SIGNAL('triggered()'), self, QtCore.SLOT('open()'))
		self.connect(self.ui.actionNext, QtCore.SIGNAL('triggered()'), self, QtCore.SLOT('next()'))
		self.connect(self.ui.actionPrev, QtCore.SIGNAL('triggered()'), self, QtCore.SLOT('prev()'))
		self.connect(self.ui.rangeSlider, QtCore.SIGNAL('valueChanged(int)'), self.setPos)

		if len(sys.argv)==2:
			QtCore.QTimer.singleShot(10, self.loadFileArgv)

	@QtCore.pyqtSlot()
	def open(self):
		fileDialog = QtGui.QFileDialog(self, "Open file")
		if fileDialog.exec_():
			self.loadFile(fileDialog.selectedFiles()[0])

	def loadFileArgv(self):
		self.loadFile(sys.argv[1])

	def loadFile(self,fname):
		self.fname = fname
		self.kbb = KBB.KBB_factory(fname)
		self.packet_count = os.stat(self.fname).st_size / 512
		self.rangeSlider.setMaximum(self.packet_count)
		self.rangeSpinBox.setMaximum(self.packet_count)
		self.setWindowTitle("KBB_Viewer: %s"%self.fname)

	@QtCore.pyqtSlot()
	def next(self):
		self.rangeSlider.triggerAction(QtGui.QAbstractSlider.SliderSingleStepAdd)

	@QtCore.pyqtSlot()
	def prev(self):
		self.rangeSlider.triggerAction(QtGui.QAbstractSlider.SliderSingleStepSub)

	@QtCore.pyqtSlot()
	def setPos(self,pos):
		self.kbb.set_index(pos)
		if not self.kbb.read_next():
			return

		# header
		self.header_preamble.setText(fmt_h32(self.kbb.header.preamble))
		self.header_version.setText(fmt_i(self.kbb.header.version))
		self.header_checksum.setText(fmt_h8(self.kbb.header.checksum))

		self.header_msg_size.setText(fmt_i(self.kbb.header.msg_size))
		self.header_msg_num.setText(fmt_i(self.kbb.header.msg_num))
		self.header_write_errors.setText(fmt_i(self.kbb.header.write_errors))

		# LTC
		ltc = "%02d:%02d:%02d.%02d"%(self.kbb.ltc.hours,self.kbb.ltc.mins,self.kbb.ltc.secs,self.kbb.ltc.frame)
		self.ltc_ltc.setText(ltc)

		# RTC
		rtc = "%04d-%02d-%02d %02d:%02d:%02d"%(self.kbb.rtc.year+1900,self.kbb.rtc.mon+1,self.kbb.rtc.mday,\
			self.kbb.rtc.hour,self.kbb.rtc.min,self.kbb.rtc.sec)
		self.rtc_rtc.setText(rtc)

		# text area
		self.formatHex()
		print pos

	def formatHex(self):
		s = ""
		offset = self.kbb.get_index()*self.kbb.SIZE
		for sidx in range(512/16):
			s += "%08X : "%(offset+sidx*16)
			for i in range(16):
				s += "%02x "%ord(self.kbb.msg[sidx*16+i])

			s += "\n"

		self.hex_view.setPlainText(s)



app = QtGui.QApplication(sys.argv)
print sys.argv
mainWindow = KBB_Viewer()
mainWindow.show()
sys.exit(app.exec_())
