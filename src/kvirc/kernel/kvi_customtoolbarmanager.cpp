//=============================================================================
//
//   File : kvi_customtoolbarmanager.cpp
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

#define __KVIRC__

#include "kvi_customtoolbarmanager.h"
#include "kvi_customtoolbardescriptor.h"
#include "kvi_config.h"
#include "kvi_locale.h"

KviCustomToolBarManager * KviCustomToolBarManager::m_pInstance = 0;

KviCustomToolBarManager::KviCustomToolBarManager()
{
	m_pDescriptors = new KviPointerHashTable<QString,KviCustomToolBarDescriptor>(17,false);
	m_pDescriptors->setAutoDelete(true);
}

KviCustomToolBarManager::~KviCustomToolBarManager()
{
	delete m_pDescriptors;
}

KviCustomToolBar * KviCustomToolBarManager::firstExistingToolBar()
{
	KviPointerHashTableIterator<QString,KviCustomToolBarDescriptor> it(*m_pDescriptors);
	while(KviCustomToolBarDescriptor * d = it.current())
	{
		if(d->toolBar())return d->toolBar();
		++it;
	}
	return 0;
}

void KviCustomToolBarManager::init()
{
	if(!m_pInstance)m_pInstance = new KviCustomToolBarManager();
}

void KviCustomToolBarManager::done()
{
	if(m_pInstance)
	{
		delete m_pInstance;
		m_pInstance = 0;
	}
}

QString KviCustomToolBarManager::idForNewToolBar(const QString &szTemplate)
{
	QString s;
	QString szTT = szTemplate.toLower();
	szTT.remove(" ");
	szTT.remove("$tr");
	szTT.remove("(");
	szTT.remove(")");
	szTT.remove("\"");
	int idx = 0;
	for(;;)
	{
		s = szTT;
		if(idx > 0)
		{
			QString tmp;
			tmp.setNum(idx);
			s += tmp;
		}
		if(!m_pDescriptors->find(s))return s;
		idx++;
	}
	return s;
}

KviCustomToolBarDescriptor * KviCustomToolBarManager::findDescriptorByInternalId(int id)
{
	KviPointerHashTableIterator<QString,KviCustomToolBarDescriptor> it(*m_pDescriptors);
	while(KviCustomToolBarDescriptor * d = it.current())
	{
		if(d->internalId() == id)return d;
		++it;
	}
	return 0;
}

bool KviCustomToolBarManager::renameDescriptor(const QString &szId,const QString &szNewId,const QString &szNewLabelCode)
{
	KviCustomToolBarDescriptor * d = m_pDescriptors->find(szId);
	if(!d)return false;
	d->rename(szNewLabelCode);
	if(szId == szNewId)return true; // already done
	m_pDescriptors->setAutoDelete(false);
	m_pDescriptors->remove(szId);
	m_pDescriptors->replace(szNewId,d);
	m_pDescriptors->setAutoDelete(true);
	return true;
}

bool KviCustomToolBarManager::destroyDescriptor(const QString &szId)
{
	KviCustomToolBarDescriptor * d = m_pDescriptors->find(szId);
	if(!d)return false;
	m_pDescriptors->remove(szId); // will delete it too!
	return true;
}

void KviCustomToolBarManager::clear()
{
	m_pDescriptors->clear(); // bye!
}

KviCustomToolBarDescriptor * KviCustomToolBarManager::create(const QString &szId,const QString &szLabelCode)
{
	KviCustomToolBarDescriptor * d = m_pDescriptors->find(szId);
	if(d)return d;
	d = new KviCustomToolBarDescriptor(szId,szLabelCode);
	m_pDescriptors->replace(szId,d);
	return d;
}

void KviCustomToolBarManager::storeVisibilityState()
{
	KviPointerHashTableIterator<QString,KviCustomToolBarDescriptor> it(*m_pDescriptors);
	while(KviCustomToolBarDescriptor * d = it.current())
	{
		d->m_bVisibleAtStartup = d->toolBar() != 0;
		++it;
	}

}

int KviCustomToolBarManager::visibleToolBarCount()
{
	int cnt = 0;
	KviPointerHashTableIterator<QString,KviCustomToolBarDescriptor> it(*m_pDescriptors);
	while(KviCustomToolBarDescriptor * d = it.current())
	{
		if(d->toolBar() != 0)cnt++;
		++it;
	}
	return cnt;
}

void KviCustomToolBarManager::createToolBarsVisibleAtStartup()
{
	KviPointerHashTableIterator<QString,KviCustomToolBarDescriptor> it(*m_pDescriptors);
	while(KviCustomToolBarDescriptor * d = it.current())
	{
		if(d->m_bVisibleAtStartup && (!d->toolBar()))
			d->createToolBar();
		++it;
	}
}

void KviCustomToolBarManager::updateVisibleToolBars()
{
	KviPointerHashTableIterator<QString,KviCustomToolBarDescriptor> it(*m_pDescriptors);
	while(KviCustomToolBarDescriptor * d = it.current())
	{
		if(d->toolBar())d->updateToolBar();
		++it;
	}
}

void KviCustomToolBarManager::load(const QString &szFileName)
{
	KviConfig cfg(szFileName,KviConfig::Read);

	KviConfigIterator it(*(cfg.dict()));
	while(it.current())
	{
		cfg.setGroup(it.currentKey());
		KviCustomToolBarDescriptor * d = new KviCustomToolBarDescriptor(it.currentKey(),QString::null);
		d->m_bVisibleAtStartup = (cfg.readIntEntry("Visible",0) > 0);
		if(!d->load(&cfg))delete d;
		else m_pDescriptors->replace(it.currentKey(),d);
		++it;
	}
}

void KviCustomToolBarManager::save(const QString &szFileName)
{
	KviConfig cfg(szFileName,KviConfig::Write);
	KviPointerHashTableIterator<QString,KviCustomToolBarDescriptor> it(*m_pDescriptors);
	while(KviCustomToolBarDescriptor * d = it.current())
	{
		cfg.setGroup(d->id());
		cfg.writeEntry("Visible",d->m_bVisibleAtStartup ? 1 : 0);
		d->save(&cfg);
		++it;
	}
}


// THIS IS A COMPATIBILITY ENTRY ADDED AT 3.0.2 TIME THAT SHOULD BE DROPPED IN A COUPLE OF VERSION BUMPS!
#ifdef SCRIPTTOOLBAR_COMPAT

#include <QDir>
void KviCustomToolBarManager::loadScripttoolbarsCompat(const QString &szFileName)
{
	KviConfig cfg(szFileName,KviConfig::Read);
	
	unsigned int cnt = cfg.readUIntEntry("Count",0);
	KviStr tmp;
	
	for(unsigned int i=0;i<cnt;i++)
	{
		tmp.sprintf("%d",i);
		KviCustomToolBarDescriptor * d = new KviCustomToolBarDescriptor(QString::null,QString::null);
		if(!d->loadScripttoolbarCompat(tmp.ptr(),&cfg))delete d;
		else m_pDescriptors->replace(d->id(),d);
	}

	// get rid of the file now...
	QString szNewFileName = szFileName + ".old";
	QDir d;
	d.rename(szFileName,szNewFileName);
}
#endif
