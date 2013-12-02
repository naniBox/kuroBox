/*
	kuroBox / naniBox
	Copyright (c) 2013
	david morris-oliveros // dmo@nanibox.com // naniBox.com

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
*/

//-----------------------------------------------------------------------------
#include "kbbviewer.h"
#include "ui_kbbviewer.h"
#include <QFileDialog>
#include <QMessageBox>

//-----------------------------------------------------------------------------
QChar _0 = QChar('0');

//-----------------------------------------------------------------------------
KBBViewer::KBBViewer(QWidget *parent) :
    QMainWindow(parent),
	ui(new Ui::KBBViewer),
	m_kbb(NULL)
{
    ui->setupUi(this);

	m_threeD = new ThreeDWidget(ui->threeD_tab);
	ui->threeD_frame_layout->addWidget(m_threeD);;

	connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(fileOpen()));
	connect(ui->actionStep_Forward, SIGNAL(triggered()), this, SLOT(stepForward()));
	connect(ui->actionStep_Backwards, SIGNAL(triggered()), this, SLOT(stepBackwards()));
	connect(ui->action_Raw_View, SIGNAL(triggered()), this, SLOT(setTabRawView()));
	connect(ui->action_3D_View, SIGNAL(triggered()), this, SLOT(setTabThreeDView()));

	connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));

	connect(this, SIGNAL(fileSizeChanged(int, int)), ui->frame_slider, SLOT(setRange(int, int)));
	connect(this, SIGNAL(filePositionChanged(int)), ui->frame_slider, SLOT(setValue(int)));
	connect(this, SIGNAL(filePositionChanged(int)), ui->frame_spin, SLOT(setValue(int)));


	connect(ui->frame_slider, SIGNAL(valueChanged(int)), this, SLOT(setFilePosition(int)));
	connect(ui->frame_spin, SIGNAL(valueChanged(int)), this, SLOT(setFilePosition(int)));

	connect(ui->threeD_tool_reset_view_btn, SIGNAL(clicked()), m_threeD, SLOT(resetView()));

	QList<QLayout*> layouts = ui->infoArea_widget->findChildren<QLayout *>();
	for ( QList<QLayout*>::iterator it = layouts.begin() ; it != layouts.end() ; ++it )
	{
		QLayout * w = *it;
		w->setContentsMargins(0, 6, 0, 0);
		w->setSpacing(4);
	}

	QList<QGroupBox*> groupboxes = ui->infoArea_widget->findChildren<QGroupBox *>();
	for ( QList<QGroupBox*>::iterator it = groupboxes.begin() ; it != groupboxes.end() ; ++it )
	{
		QGroupBox * gb = *it;
		if ( !gb->isCheckable() )
			continue;
		QList<QWidget*> widgets = gb->findChildren<QWidget*>(QRegExp(".*_widget"));
		ui->hexView_text->appendPlainText(QString("This groupBox (%1) has %2 widgets!\n")
										  .arg(gb->objectName())
										  .arg(widgets.end()-widgets.begin()));
		for ( QList<QWidget*>::const_iterator wit = widgets.begin(); wit != widgets.end() ; ++wit)
		{
			QWidget * w = *wit;
			connect(gb, SIGNAL(toggled(bool)), w, SLOT(setVisible(bool)));
			ui->hexView_text->appendPlainText(QString("\t'%1'\n")
												.arg(w->objectName()));
		}
	}
	groupboxes = ui->infoArea_widget->findChildren<QGroupBox *>(QRegExp("_02_.*_groupBox"));
	for ( QList<QGroupBox*>::iterator it = groupboxes.begin() ; it != groupboxes.end() ; ++it )
		(*it)->hide();
}

//-----------------------------------------------------------------------------
KBBViewer::~KBBViewer()
{
    delete ui;
}

//-----------------------------------------------------------------------------
void KBBViewer::about()
{
	QMessageBox msgBox;
	msgBox.setText("KBB Viewer\n"
				   "Copyright (c) 2013 - naniBox.com\n"
				   "Build Date: " __DATE__ " at " __TIME__);
	msgBox.exec();
}

//-----------------------------------------------------------------------------
void KBBViewer::fileOpen()
{
	QString fname = QFileDialog::getOpenFileName(this, tr("Open KBB file"), "c:/Users/talsit/Code/naniBox/KBBViewer/KURO0015.KBB", tr("KBB Files (*.kbb)"));
	if ( !fname.isNull() && m_fname != fname )
	{
		m_fname = fname;
		if ( m_kbb )
			delete m_kbb;
		m_kbb = new KBB();
		if ( m_kbb->open(m_fname.toStdString()) )
		{
			m_position = 0;
			ui->hexView_text->appendPlainText("Opened: "+m_fname);
			uint32_t packets = m_kbb->size();
			emit fileSizeChanged(0, packets);
			// why is this not a slot?!
			ui->frame_spin->setRange(0, packets);
			emit fileOpened(m_fname);
			// go to the first packet after the header
			setFilePosition(1);
		}
	}
}

//-----------------------------------------------------------------------------
void KBBViewer::setFilePosition(int pos)
{
	if ( !m_kbb )
		return;

	if ( pos < 0 )
		return;

	if ( pos >= m_kbb->size() )
		return;

	if ( pos == m_position )
		return;

	if ( m_kbb->seek(pos) )
	{
		m_position = pos;
		handlePacket(m_kbb->next());
		emit filePositionChanged(pos);
	}
}

//-----------------------------------------------------------------------------
void KBBViewer::stepForward()
{
	if ( !m_kbb )
		return;

	setFilePosition(m_position+1);
}

//-----------------------------------------------------------------------------
void KBBViewer::stepBackwards()
{
	if ( !m_kbb )
		return;

	setFilePosition(m_position-1);
}

//-----------------------------------------------------------------------------
void KBBViewer::setTabRawView()
{
	ui->content->setCurrentIndex(0);
}

//-----------------------------------------------------------------------------
void KBBViewer::setTabThreeDView()
{
	ui->content->setCurrentIndex(1);
}

//-----------------------------------------------------------------------------
void KBBViewer::handle_02_01(const KBB_02_01 * data)
{
	const kbb_02_01_t & _02_01 = data->packet();

	// header
	ui->_02_01_header_preamble_edit->setText(QString("0x%1").arg(_02_01.header.preamble, 8, 16, _0));
	ui->_02_01_header_checksum_edit->setText(QString("0x%1").arg(_02_01.header.checksum, 4, 16, _0));
	ui->_02_01_header_msg_class_edit->setText(QString("0x%1").arg(_02_01.header.msg_class, 2, 16, _0));
	ui->_02_01_header_msg_subclass_edit->setText(QString("0x%1").arg(_02_01.header.msg_subclass, 2, 16, _0));
	ui->_02_01_header_msg_size_edit->setText(QString("%1").arg(_02_01.header.msg_size, 0, 10));
	ui->_02_01_header_msg_num_edit->setText(QString("%1").arg(_02_01.msg_num, 0, 10));
	ui->_02_01_header_write_errors_edit->setText(QString("%1").arg(_02_01.write_errors, 0, 10));

	// TIME
	smpte_timecode_t smpte;
	ltc_frame_to_smpte_timecode(&smpte, &_02_01.ltc_frame);
	ui->_02_01_time_ltc_edit->setText(QString("%1 / %2 / %3")
							   .arg(QString("%1:%2:%3.%4")
									.arg(smpte.hours, 2, 10, _0)
									.arg(smpte.minutes, 2, 10, _0)
									.arg(smpte.seconds, 2, 10, _0)
									.arg(smpte.frames, 2, 10, _0))
							   .arg(ltc_frame_count(&smpte, 30, _02_01.ltc_frame.drop_frame_flag)) // @TODO make this an option
							   .arg(_02_01.ltc_frame.drop_frame_flag?"DF":"NDF"));

	ui->_02_01_time_rtc_edit->setText(QString("%1-%2-%3 %4:%5:%6")
							   .arg(_02_01.rtc.tm_year+1900, 4, 10, _0)
							   .arg(_02_01.rtc.tm_mon+1, 2, 10, _0)
							   .arg(_02_01.rtc.tm_mday, 2, 10, _0)
							   .arg(_02_01.rtc.tm_hour, 2, 10, _0)
							   .arg(_02_01.rtc.tm_min, 2, 10, _0)
							   .arg(_02_01.rtc.tm_sec, 2, 10, _0));

	// GPS
	float lon,lat,alt;
	lon = lat = alt = 0.0f;
	ecef_to_lla(_02_01.nav_sol.ecefX, _02_01.nav_sol.ecefY, _02_01.nav_sol.ecefZ, lon, lat, alt);
	ui->_02_01_gps_lon_edit->setText(QString("%1")
							  .arg((double)lon, 9, 'f', 9));
	ui->_02_01_gps_lat_edit->setText(QString("%1")
							  .arg((double)lat, 9, 'f', 9));
	ui->_02_01_gps_alt_edit->setText(QString("%1m")
							  .arg((double)alt, 9, 'f', 2));
	ui->_02_01_gps_pps_edit->setText(QString("%1")
							  .arg(_02_01.pps));
	ui->_02_01_gps_gpsfix_edit->setText(QString("%1")
								 .arg(_02_01.nav_sol.gpsfix));
	ui->_02_01_gps_gpsfix_edit->setStyleSheet(( _02_01.nav_sol.gpsfix == 3 )?
										   "QLineEdit { background-color: rgb(212, 255, 212); }":
											"QLineEdit { background-color: rgb(255, 212, 212); }");


	// GPS Details
	ui->_02_01_gps_detail_itow_edit->setText(QString("%1").arg(_02_01.nav_sol.itow));
	ui->_02_01_gps_detail_ftow_edit->setText(QString("%1").arg(_02_01.nav_sol.ftow));
	ui->_02_01_gps_detail_week_edit->setText(QString("%1").arg(_02_01.nav_sol.week));
	ui->_02_01_gps_detail_flags_edit->setText(QString("0x%1").arg(_02_01.nav_sol.flags, 2, 16, _0));
	ui->_02_01_gps_detail_ecef_edit->setText(QString("%1, %2, %3").arg(_02_01.nav_sol.ecefX).arg(_02_01.nav_sol.ecefY).arg(_02_01.nav_sol.ecefZ));
	ui->_02_01_gps_detail_ecefv_edit->setText(QString("%1, %2, %3").arg(_02_01.nav_sol.ecefVX).arg(_02_01.nav_sol.ecefVY).arg(_02_01.nav_sol.ecefVZ));
	ui->_02_01_gps_detail_pacc_edit->setText(QString("%1 / %2").arg(_02_01.nav_sol.pAcc).arg(_02_01.nav_sol.sAcc));
	ui->_02_01_gps_detail_pdop_edit->setText(QString("%1 / %2").arg(_02_01.nav_sol.pdop).arg(_02_01.nav_sol.numSV));

	// VectorNav
	ui->_02_01_vn_yaw_edit->setText(QString("%1").arg(_02_01.vnav.ypr[0], 0, 'f', 5));
	ui->_02_01_vn_pitch_edit->setText(QString("%1").arg(_02_01.vnav.ypr[1], 0, 'f', 5));
	ui->_02_01_vn_roll_edit->setText(QString("%1").arg(_02_01.vnav.ypr[2], 0, 'f', 5));
	ui->_02_01_vn_timestamp_edit->setText(QString("%1").arg(_02_01.vnav.ypr_ts));

	// Hex View
	drawHexView(reinterpret_cast<const uint8_t *>(&_02_01), KBB_MSG_SIZE);

	m_threeD->setYPR(_02_01.vnav.ypr[0], _02_01.vnav.ypr[1], _02_01.vnav.ypr[2]);
	ui->_02_01_groupBox->show();
}

//-----------------------------------------------------------------------------
void KBBViewer::handle_02_02(const KBB_02_02 * data)
{
	const kbb_02_02_t & _02_02 = data->packet();

	// header
	ui->_02_02_header_preamble_edit->setText(QString("0x%1").arg(_02_02.header.preamble, 8, 16, _0));
	ui->_02_02_header_checksum_edit->setText(QString("0x%1").arg(_02_02.header.checksum, 4, 16, _0));
	ui->_02_02_header_msg_class_edit->setText(QString("0x%1").arg(_02_02.header.msg_class, 2, 16, _0));
	ui->_02_02_header_msg_subclass_edit->setText(QString("0x%1").arg(_02_02.header.msg_subclass, 2, 16, _0));
	ui->_02_02_header_msg_size_edit->setText(QString("%1").arg(_02_02.header.msg_size, 0, 10));
	ui->_02_02_header_msg_num_edit->setText(QString("%1").arg(_02_02.msg_num, 0, 10));
	ui->_02_02_header_write_errors_edit->setText(QString("%1").arg(_02_02.write_errors, 0, 10));

	// TIME
	smpte_timecode_t smpte;
	ltc_frame_to_smpte_timecode(&smpte, &_02_02.ltc_frame);
	ui->_02_02_time_ltc_edit->setText(QString("%1 / %2 / %3")
							   .arg(QString("%1:%2:%3.%4")
									.arg(smpte.hours, 2, 10, _0)
									.arg(smpte.minutes, 2, 10, _0)
									.arg(smpte.seconds, 2, 10, _0)
									.arg(smpte.frames, 2, 10, _0))
							   .arg(ltc_frame_count(&smpte, 30, _02_02.ltc_frame.drop_frame_flag)) // @TODO make this an option
							   .arg(_02_02.ltc_frame.drop_frame_flag?"DF":"NDF"));

	ui->_02_02_time_internal_edit->setText(QString("%1:%2:%3.%4")
										 .arg(_02_02.smpte_time.hours, 2, 10, _0)
										 .arg(_02_02.smpte_time.minutes, 2, 10, _0)
										 .arg(_02_02.smpte_time.seconds, 2, 10, _0)
										 .arg(_02_02.smpte_time.frames, 2, 10, _0));

	ui->_02_02_time_rtc_edit->setText(QString("%1-%2-%3 %4:%5:%6")
							   .arg(_02_02.rtc.tm_year+1900, 4, 10, _0)
							   .arg(_02_02.rtc.tm_mon+1, 2, 10, _0)
							   .arg(_02_02.rtc.tm_mday, 2, 10, _0)
							   .arg(_02_02.rtc.tm_hour, 2, 10, _0)
							   .arg(_02_02.rtc.tm_min, 2, 10, _0)
							   .arg(_02_02.rtc.tm_sec, 2, 10, _0));

	// GPS
	float lon,lat,alt;
	lon = lat = alt = 0.0f;
	ecef_to_lla(_02_02.nav_sol.ecefX, _02_02.nav_sol.ecefY, _02_02.nav_sol.ecefZ, lon, lat, alt);
	ui->_02_02_gps_lon_edit->setText(QString("%1")
							  .arg((double)lon, 9, 'f', 9));
	ui->_02_02_gps_lat_edit->setText(QString("%1")
							  .arg((double)lat, 9, 'f', 9));
	ui->_02_02_gps_alt_edit->setText(QString("%1m")
							  .arg((double)alt, 9, 'f', 2));
	ui->_02_02_gps_pps_edit->setText(QString("%1")
							  .arg(_02_02.pps));
	ui->_02_02_gps_gpsfix_edit->setText(QString("%1")
								 .arg(_02_02.nav_sol.gpsfix));
	ui->_02_02_gps_gpsfix_edit->setStyleSheet(( _02_02.nav_sol.gpsfix == 3 )?
										   "QLineEdit { background-color: rgb(212, 255, 212); }":
											"QLineEdit { background-color: rgb(255, 212, 212); }");


	// GPS Details
	ui->_02_02_gps_detail_itow_edit->setText(QString("%1").arg(_02_02.nav_sol.itow));
	ui->_02_02_gps_detail_ftow_edit->setText(QString("%1").arg(_02_02.nav_sol.ftow));
	ui->_02_02_gps_detail_week_edit->setText(QString("%1").arg(_02_02.nav_sol.week));
	ui->_02_02_gps_detail_flags_edit->setText(QString("0x%1").arg(_02_02.nav_sol.flags, 2, 16, _0));
	ui->_02_02_gps_detail_ecef_edit->setText(QString("%1, %2, %3").arg(_02_02.nav_sol.ecefX).arg(_02_02.nav_sol.ecefY).arg(_02_02.nav_sol.ecefZ));
	ui->_02_02_gps_detail_ecefv_edit->setText(QString("%1, %2, %3").arg(_02_02.nav_sol.ecefVX).arg(_02_02.nav_sol.ecefVY).arg(_02_02.nav_sol.ecefVZ));
	ui->_02_02_gps_detail_pacc_edit->setText(QString("%1 / %2").arg(_02_02.nav_sol.pAcc).arg(_02_02.nav_sol.sAcc));
	ui->_02_02_gps_detail_pdop_edit->setText(QString("%1 / %2").arg(_02_02.nav_sol.pdop).arg(_02_02.nav_sol.numSV));

	// VectorNav
	ui->_02_02_vn_yaw_edit->setText(QString("%1").arg(_02_02.vnav.ypr[0], 0, 'f', 5));
	ui->_02_02_vn_pitch_edit->setText(QString("%1").arg(_02_02.vnav.ypr[1], 0, 'f', 5));
	ui->_02_02_vn_roll_edit->setText(QString("%1").arg(_02_02.vnav.ypr[2], 0, 'f', 5));
	ui->_02_02_vn_timestamp_edit->setText(QString("%1").arg(_02_02.vnav.ypr_ts));

	// Hex View
	drawHexView(reinterpret_cast<const uint8_t *>(&_02_02), KBB_MSG_SIZE);

	m_threeD->setYPR(_02_02.vnav.ypr[0], _02_02.vnav.ypr[1], _02_02.vnav.ypr[2]);
	ui->_02_02_groupBox->show();
}

//-----------------------------------------------------------------------------
void KBBViewer::drawHexView(const uint8_t * data, uint32_t len)
{
	QString hex;
	uint32_t offset = m_position * KBB_MSG_SIZE;

	hex += QString("Offset     Index  ");
	for ( int j = 0 ; j < 16 ; j++ )
		hex += QString("%1 ").arg(j, 2, 16, _0);
	hex += "\n-----------------------------------------------------------------\n";

	for ( int i = 0 ; i < KBB_MSG_SIZE/16 ; i++ )
	{
		hex += QString("%1 : ").arg(offset+i*16, 8, 16, _0);
		hex += QString("%1 : ").arg(i*16, 4, 16, _0);
		for ( int j = 0 ; j < 16 ; j++ )
			hex += QString("%1 ").arg(data[i*16+j], 2, 16, _0);
		hex += "\n";
	}

	ui->hexView_text->setPlainText(hex);
}

//-----------------------------------------------------------------------------
void KBBViewer::handlePacket(const KBB_Packet * packet)
{

	const kbb_header_t & h = packet->header();
	switch( h.msg_class )
	{
		case KBB_CLASS_HEADER:
			ui->hexView_text->setPlainText(QString("Header Packet: class 0x%1, subclass: 0x%2").arg(packet->header().msg_class, 2, 16).arg(packet->header().msg_subclass, 2, 16));
			break;
		case KBB_CLASS_DATA:
		{
			QList<QGroupBox*> groupboxes = ui->infoArea_widget->findChildren<QGroupBox *>(QRegExp("_02_.*_groupBox"));
			for ( QList<QGroupBox*>::iterator it = groupboxes.begin() ; it != groupboxes.end() ; ++it )
				(*it)->hide();
			switch( h.msg_subclass )
			{
				case KBB_SUBCLASS_DATA_01:
				{
					const KBB_02_01 * data = dynamic_cast<const KBB_02_01 *>(packet);
					if ( data )
						handle_02_01(data);
				} break;
				case KBB_SUBCLASS_DATA_02:
				{
					const KBB_02_02 * data = dynamic_cast<const KBB_02_02 *>(packet);
					if ( data )
						handle_02_02(data);
				} break;
			}
		} break;
	}
}
