#ifndef _SOCKETSPYWINDOW_H_
#define _SOCKETSPYWINDOW_H_
//=============================================================================
//
//   File : socketspywindow.h
//   Creation date : Sun Nov 26 2000 13:13:00 CEST by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2000-2008 Szymon Stefanek (pragma at kvirc dot net)
//
//   This program is FREE software. You can redistribute it and/or
//   modify it under the socketspys of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your opinion) any later version.
//
//   This program is distributed in the HOPE that it will be USEFUL,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program. If not, write to the Free Software Foundation,
//   Inc. ,59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//=============================================================================

#include "kvi_window.h"
#include "kvi_string.h"
#include "kvi_ircdatastreammonitor.h"

class KviConsole;

class KviSocketSpyWindow : public KviWindow, public KviIrcDataStreamMonitor
{
	Q_OBJECT
public:
	KviSocketSpyWindow(KviFrame * lpFrm,KviConsole * lpConsole);
	~KviSocketSpyWindow();
protected:
	virtual QPixmap * myIconPtr();
	virtual void fillCaptionBuffers();
	virtual void resizeEvent(QResizeEvent *e);
	virtual void getBaseLogFileName(QString &buffer);
	virtual void applyOptions();
public:
	virtual QSize sizeHint() const;
	virtual void incomingMessage(const char * message);
	// message is NOT null terminated! For proxy connections it might spit out binary data!
	virtual void outgoingMessage(const char * message,int len);
	virtual void connectionInitiated();
	virtual void connectionTerminated();
	virtual void die();
};

#endif //_KVI_SOCKETSPYWINDOW_H_
