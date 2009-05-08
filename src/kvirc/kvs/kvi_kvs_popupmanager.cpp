//=============================================================================
//
//   File : kvi_kvs_popupmanager.cpp
//   Creation date : Tue 7 Jan 2004 02:11:41 by Szymon Stefanek
//
//   This file is part of the KVIrc IRC client distribution
//   Copyright (C) 2004-2008 Szymon Stefanek <pragma at kvirc dot net>
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

#include "kvi_kvs_popupmanager.h"
#include "kvi_locale.h"
#include "kvi_config.h"

KviKvsPopupManager * KviKvsPopupManager::m_pInstance = 0;

KviKvsPopupManager::KviKvsPopupManager()
{
	m_pInstance = this;
	m_pPopupDict = new KviPointerHashTable<QString,KviKvsPopupMenu>(17,false);
	m_pPopupDict->setAutoDelete(true);
}

KviKvsPopupManager::~KviKvsPopupManager()
{
	delete m_pPopupDict;
}

void KviKvsPopupManager::init()
{
	if(KviKvsPopupManager::instance())
	{
		debug("WARNING: Trying to create the KviKvsPopupManager twice!");
		return;
	}
	(void)new KviKvsPopupManager();
}

void KviKvsPopupManager::done()
{
	if(!KviKvsPopupManager::instance())
	{
		debug("WARNING: Trying to destroy the KviKvsPopupManager twice!");
		return;
	}
	delete KviKvsPopupManager::instance();
}

KviKvsPopupMenu * KviKvsPopupManager::get(const QString &szPopupName)
{
	KviKvsPopupMenu * m = lookup(szPopupName);
	if(!m)
	{
		m = new KviKvsPopupMenu(szPopupName);
		add(szPopupName,m);
	}
	return m;
}

void KviKvsPopupManager::load(const QString &szFileName)
{
	m_pPopupDict->clear();
	KviConfig cfg(szFileName,KviConfig::Read);

	KviConfigIterator it(*(cfg.dict()));

	KviPointerList<QString> l;
	l.setAutoDelete(true);

	while(it.current())
	{
		l.append(new QString(it.currentKey()));
		++it;
	}

	for(QString * s = l.first();s;s = l.next())
	{
		cfg.setGroup(*s);
		KviKvsPopupMenu * m = new KviKvsPopupMenu(*s);
		m->load("",&cfg);
		m_pPopupDict->insert(*s,m);
		//++it;
	}
}

void KviKvsPopupManager::save(const QString &szFileName)
{
	KviConfig cfg(szFileName,KviConfig::Write);
	cfg.clear();

	KviPointerHashTableIterator<QString,KviKvsPopupMenu> it(*m_pPopupDict);
	while(it.current())
	{
		cfg.setGroup(it.current()->popupName());
		it.current()->save("",&cfg);
		++it;
	}
}
