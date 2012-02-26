#ifndef _KVI_MENUBAR_H_
#define _KVI_MENUBAR_H_

//=============================================================================
//
//   File : KviMenuBar.h
//   Creation date : Wen Jun 21 2000 13:11:24 by Szymon Stefanek
//
//   This file is part of the KVIrc irc client distribution
//   Copyright (C) 2000-2010 Szymon Stefanek (pragma at kvirc dot net)
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

#include "kvi_settings.h"
#include "KviPointerList.h"
#include "KviCString.h"
#include "KviTalMenuBar.h"

class KviMainWindow;
class QMenu;
class KviKvsPopupMenu;

typedef struct _KviScriptMenuBarItem
{
	KviCString            szPopupName;
	KviKvsPopupMenu * pPopup;
	KviCString            szText;
} KviScriptMenuBarItem;


class KVIRC_API KviMenuBar : public KviTalMenuBar
{
	Q_OBJECT
public:
	KviMenuBar(KviMainWindow * par,const char * name);
	~KviMenuBar();
protected:
    QMenu                       * m_pToolbarsPopup;
    QMenu                       * m_pRecentServersPopup;
#ifdef COMPILE_ON_MAC
    QMenu                       * m_pHelpMenu;
#endif
	KviMainWindow                         * m_pFrm;
	KviPointerList<KviScriptMenuBarItem> * m_pScriptItemList;
protected:
	KviScriptMenuBarItem * findMenu(const QString &text);
	KviScriptMenuBarItem * findMenu(KviKvsPopupMenu * p);
    void addDefaultItem(const QString &text,QMenu * pop);
public:
	int getDefaultItemRealIndex(int iDefaultIndex);
	void addMenu(const QString &text,KviKvsPopupMenu * p,int index);
	bool removeMenu(const QString &text);
protected slots:
	void menuDestroyed();
	void setupMainPopup();
	void setupSettingsPopup();
	void setupHelpPopup();
	void setupRecentServersPopup();
	void setupScriptingPopup();
    void newConnectionToServer(QAction *pAction);
	void setupToolbarsPopup();
	void setupToolsPopup();
    void toolsPopupSelected(QAction *pAction);
    void actionTriggered(QAction *pAction);
    void actionTriggered(bool);
    void createMacNativeItems();
};

#endif //_KVI_MENUBAR_H_
