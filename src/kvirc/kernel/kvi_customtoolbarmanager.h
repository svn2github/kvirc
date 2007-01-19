#ifndef _KVI_CUSTOMTOOLBARMANAGER_H_
#define _KVI_CUSTOMTOOLBARMANAGER_H_
//=============================================================================
//
//   File : kvi_customtoolbarmanager.h
//   Created on Sun 05 Dec 2004 18:20:18 by Szymon Stefanek
//
//   This file is part of the KVIrc IRC client distribution
//   Copyright (C) 2004 Szymon Stefanek <pragma at kvirc dot net>
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
//=============================================================================

#include "kvi_settings.h"
#include "kvi_dict.h"

#ifdef COMPILE_ON_WINDOWS
	#include "kvi_customtoolbardescriptor.h"
#else
	class KviCustomToolBarDescriptor;
#endif

class KviCustomToolBar;

// THIS IS A COMPATIBILITY ENTRY ADDED AT 3.0.2 TIME THAT SHOULD BE DROPPED IN A COUPLE OF VERSION BUMPS!
#define SCRIPTTOOLBAR_COMPAT 1

class KVIRC_API KviCustomToolBarManager
{
protected:
	KviCustomToolBarManager();
	~KviCustomToolBarManager();
protected:
	static KviCustomToolBarManager * m_pInstance;
	KviDict<KviCustomToolBarDescriptor> * m_pDescriptors;
public:
	static KviCustomToolBarManager * instance(){ return m_pInstance; };
	static void init();
	static void done();
	void clear();
	int descriptorCount(){ return m_pDescriptors->count(); };
	int visibleToolBarCount();
	QString idForNewToolBar(const QString &szTemplate);
	KviDict<KviCustomToolBarDescriptor> * descriptors(){ return m_pDescriptors; };
	KviCustomToolBar * firstExistingToolBar();
	KviCustomToolBarDescriptor * create(const QString &szId,const QString &szLabelCode);
	KviCustomToolBarDescriptor * find(const QString &szId){ return m_pDescriptors->find(szId); };
	KviCustomToolBarDescriptor * findDescriptorByInternalId(int id);
	void updateVisibleToolBars();
	void createToolBarsVisibleAtStartup();
	void storeVisibilityState();
	bool renameDescriptor(const QString &szId,const QString &szNewId,const QString &szNewLabelCode);
	bool destroyDescriptor(const QString &szId);
	void load(const QString &szFileName);
	// THIS IS A COMPATIBILITY ENTRY ADDED AT 3.0.2 TIME THAT SHOULD BE DROPPED IN A COUPLE OF VERSION BUMPS!
#ifdef SCRIPTTOOLBAR_COMPAT
	void loadScripttoolbarsCompat(const QString &szFileName);
#endif
	void save(const QString &szFileName);
};


#endif //!_KVI_CUSTOMTOOLBARMANAGER_H_
