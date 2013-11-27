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
#ifndef KBBVIEWER_H
#define KBBVIEWER_H

//-----------------------------------------------------------------------------
#include <QMainWindow>
#include "KBB/KBB.h"
#include "ThreeDWidget.h"

//-----------------------------------------------------------------------------
namespace Ui {
class KBBViewer;
}

//-----------------------------------------------------------------------------
class KBBViewer : public QMainWindow
{
    Q_OBJECT

public:
    explicit KBBViewer(QWidget *parent = 0);
    ~KBBViewer();

public slots:
	void about();
	void fileOpen();
	void setFilePosition(int pos);
	void stepForward();
	void stepBackwards();
	void setTabRawView();
	void setTabThreeDView();

signals:
	void fileOpened(const QString & fname);
	void fileSizeChanged(int min, int max);
	void filePositionChanged(int pos);

private:
	void handlePacket(const KBB_Packet * packet);

    Ui::KBBViewer *ui;

	ThreeDWidget * m_threeD;

	QString m_fname;
	KBB * m_kbb;
	uint32_t m_position;
};

#endif // KBBVIEWER_H
