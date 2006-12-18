//
//   File : kvi_tal_toolbar.cpp
//   Creation date : Mon Aug 13 05:05:45 2001 GMT by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2001 Szymon Stefanek (pragma at kvirc dot net)
//
//   This program is FREE software. You can redistribute it and/or
//   modify it under the terms of the GNU General Public License
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

#define __KVILIB__


#include "kvi_tal_toolbar.h"

#ifdef COMPILE_KDE_SUPPORT

	KviTalToolBar::KviTalToolBar(const QString &label,QMainWindow *w,QT_TOOLBARDOCK_TYPE dock,bool bNewLine,const char * nam)
	: KToolBar(w,dock,bNewLine,nam)
	{
		setLabel(label);
	}

	KviTalToolBar::~KviTalToolBar()
	{
	}

	#include "kvi_tal_toolbar_kde.moc"

#else

	KviTalToolBar::KviTalToolBar(const QString &label,QMainWindow *w,QT_TOOLBARDOCK_TYPE dock,bool bNewLine,const char * nam)
	: QToolBar(label,w,dock,bNewLine,nam)
	{
	}

	KviTalToolBar::~KviTalToolBar()
	{
	}

	#include "kvi_tal_toolbar_qt.moc"

#endif
