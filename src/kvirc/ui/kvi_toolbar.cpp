//=============================================================================
//
//   File : kvi_toolbar.cpp
//   Creation date : Tue Sep 17 02:00:17 2002 GMT by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2002-2008 Szymon Stefanek (pragma at kvirc dot net)
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
//   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
//=============================================================================

#include "kvi_toolbar.h"
#include "kvi_frame.h"
#include "kvi_locale.h"
#include "kvi_app.h"
#include "kvi_tal_popupmenu.h"

#include <QCursor>
#include <QEvent>
#include <QMouseEvent>

static KviTalPopupMenu * g_pToolBarContextPopup   = 0;
static KviTalPopupMenu * g_pToolBarWindowsPopup   = 0;
static KviTalPopupMenu * g_pToolBarIconSizesPopup = 0;
static KviTalPopupMenu * g_pToolBarPositionsPopup = 0;


KviToolBar::KviToolBar(const QString & szLabel, Qt::ToolBarArea type, bool bNewLine, const char * pcName)
: KviTalToolBar(szLabel,g_pFrame,type,bNewLine,pcName)
{
}

KviToolBar::~KviToolBar()
{
	if(g_pToolBarContextPopup)
		delete g_pToolBarContextPopup;
	if(g_pToolBarIconSizesPopup)
		delete g_pToolBarIconSizesPopup;
	if(g_pToolBarPositionsPopup)
		delete g_pToolBarPositionsPopup;
	if(g_pToolBarWindowsPopup)
		delete g_pToolBarWindowsPopup;

	g_pToolBarContextPopup   = 0;
	g_pToolBarIconSizesPopup = 0;
	g_pToolBarPositionsPopup = 0;
	g_pToolBarWindowsPopup   = 0;
}

void KviToolBar::mousePressEvent(QMouseEvent * e)
{
	if(!(e->button() & Qt::RightButton))
	{
		KviTalToolBar::mousePressEvent(e);
		return;
	}

	if(!g_pToolBarContextPopup)
		g_pToolBarContextPopup = new KviTalPopupMenu();
	if(!g_pToolBarIconSizesPopup)
		g_pToolBarIconSizesPopup = new KviTalPopupMenu();
	if(!g_pToolBarPositionsPopup)
		g_pToolBarPositionsPopup = new KviTalPopupMenu();
	if(!g_pToolBarWindowsPopup)
		g_pToolBarWindowsPopup = new KviTalPopupMenu();

	g_pToolBarContextPopup->clear();
	g_pToolBarIconSizesPopup->clear();
	g_pToolBarPositionsPopup->clear();
	g_pToolBarWindowsPopup->clear();

	g_pFrame->fillToolBarsPopup(g_pToolBarWindowsPopup);
	g_pToolBarContextPopup->insertItem(__tr2qs("Toolbars"),g_pToolBarWindowsPopup);

	g_pToolBarContextPopup->insertItem(__tr2qs("Icon Size"),g_pToolBarIconSizesPopup);

	g_pToolBarIconSizesPopup->insertItem(__tr2qs("Small (22x22)"),this,SLOT(setSmallIcons()));
	g_pToolBarIconSizesPopup->insertItem(__tr2qs("Large (32x32)"),this,SLOT(setBigIcons()));

	g_pToolBarContextPopup->popup(QCursor::pos());
}

void KviToolBar::setBigIcons()
{
	g_pFrame->setUsesBigPixmaps(true);
}

void KviToolBar::setSmallIcons()
{
	g_pFrame->setUsesBigPixmaps(false);
}

#ifndef COMPILE_USE_STANDALONE_MOC_SOURCES
#include "kvi_toolbar.moc"
#endif //COMPILE_USE_STANDALONE_MOC_SOURCES
