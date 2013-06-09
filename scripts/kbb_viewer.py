import sys
import os
import struct
from PyQt4 import QtCore, QtGui, uic
import KBB

KBB.DBG = 0


def fmt_h32(x):		return "0x%08X"%x
def fmt_i(x):		return "%i"%x
def fmt_f(x,d):		fmt="%."+"%d"%d+"f";return fmt%x
def fmt_h8(x):		return "0x%02x"%x

def fmt_preamble(kbb):return "%s / %s"%(fmt_h32(kbb.header.preamble),struct.pack("<I",kbb.header.preamble))
def fmt_checksum(kbb):return "%s / %s"%(fmt_h8(kbb.header.checksum),"valid"if kbb.valid_packet else"INVALID")

def fmt_3xi(a,b,c): return "%s, %s, %s"%(fmt_i(a),fmt_i(b),fmt_i(c))
def fmt_3xf(a,b,c,d=5): return "%s, %s, %s"%(fmt_f(a,d),fmt_f(b,d),fmt_f(c,d))
def fmt_2xi(a,b): return "%s, %s"%(fmt_i(a),fmt_i(b))

class KBB_Viewer(QtGui.QMainWindow):
	def __init__(self):
		QtGui.QMainWindow.__init__(self)
		self.ui = uic.loadUi("kbb_viewer.ui", self)
		self.connect(self.ui.actionOpen, QtCore.SIGNAL('triggered()'), self, QtCore.SLOT('open()'))
		self.connect(self.ui.actionNext, QtCore.SIGNAL('triggered()'), self, QtCore.SLOT('next()'))
		self.connect(self.ui.actionPrev, QtCore.SIGNAL('triggered()'), self, QtCore.SLOT('prev()'))
		self.connect(self.ui.rangeSlider, QtCore.SIGNAL('valueChanged(int)'), self.setPos)

		self.ui.rangeSlider.setFocus(7)
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
		self.packetCountEdit.setText("%d"%self.packet_count)
		self.setPos(0)

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
		self.kbb.check_all()

		# header
		self.header_preamble.setText(fmt_preamble(self.kbb))
		self.header_version.setText(fmt_i(self.kbb.header.version))
		self.header_checksum.setText(fmt_checksum(self.kbb))

		self.header_msg_size.setText(fmt_i(self.kbb.header.msg_size))
		self.header_msg_num.setText(fmt_i(self.kbb.header.msg_num))
		self.header_write_errors.setText(fmt_i(self.kbb.header.write_errors))

		# LTC
		ltc = "%02d:%02d:%02d.%02d"%(self.kbb.ltc.hours,self.kbb.ltc.minutes,self.kbb.ltc.seconds,self.kbb.ltc.frames)
		self.ltc_ltc.setText(ltc)

		# RTC
		rtc = "%04d-%02d-%02d %02d:%02d:%02d"%(self.kbb.rtc.year+1900,self.kbb.rtc.mon+1,self.kbb.rtc.mday,\
			self.kbb.rtc.hour,self.kbb.rtc.min,self.kbb.rtc.sec)
		self.rtc_rtc.setText(rtc)

		# NAV_SOL
		self.nav_sol_pps.setText(fmt_i(self.kbb.nav_sol.pps))
		self.nav_sol_itow.setText(fmt_i(self.kbb.nav_sol.itow))
		self.nav_sol_ftow.setText(fmt_i(self.kbb.nav_sol.ftow))
		self.nav_sol_week.setText(fmt_i(self.kbb.nav_sol.week))
		self.nav_sol_gpsfix.setText(fmt_i(self.kbb.nav_sol.gpsfix))
		self.nav_sol_flags.setText(fmt_i(self.kbb.nav_sol.flags))
		self.nav_sol_ecef.setText(fmt_3xi(self.kbb.nav_sol.ecefX,self.kbb.nav_sol.ecefY,self.kbb.nav_sol.ecefZ))
		self.nav_sol_ecefv.setText(fmt_3xi(self.kbb.nav_sol.ecefVX,self.kbb.nav_sol.ecefVY,self.kbb.nav_sol.ecefVZ))
		self.nav_sol_pacc_sacc.setText(fmt_2xi(self.kbb.nav_sol.pAcc, self.kbb.nav_sol.sAcc))
		self.nav_sol_pdop_numsv.setText(fmt_2xi(self.kbb.nav_sol.pdop, self.kbb.nav_sol.numSV))

		self.kbb.nav_sol.calculateLLA()
		self.nav_sol_lla.setText(fmt_3xf(self.kbb.nav_sol.lat,self.kbb.nav_sol.lon,self.kbb.nav_sol.alt,7))

		#self.pps,self.header,self.msg_id,self.msg_len,self.itow,self.ftow,self.week,self.gpsfix,self.flags, \
		#	self.ecefX,self.ecefY,self.ecefZ,self.pAcc,self.ecefVX,self.ecefVY,self.ecefVZ, \
		#	self.sAcc,self.pdop,self.reserved1,self.numSV,self.reserved2,self.cs = \

		# text area
		self.formatHex()
		
		print "Setting position of KBB file to", pos

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
