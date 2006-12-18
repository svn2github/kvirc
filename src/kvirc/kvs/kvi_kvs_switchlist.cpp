//=============================================================================
//
//   File : kvi_kvs_switchlist.cpp
//   Created on Mon 27 Oct 2003 03:47:48 by Szymon Stefanek
//
//   This file is part of the KVIrc IRC client distribution
//   Copyright (C) 2003 Szymon Stefanek <pragma at kvirc dot net>
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

#include "kvi_kvs_switchlist.h"

KviKvsSwitchList::KviKvsSwitchList()
{
#ifdef COMPILE_NEW_KVS
	m_pShortSwitchDict = 0;
	m_pLongSwitchDict = 0;
#endif
}

	
KviKvsSwitchList::~KviKvsSwitchList()
{
#ifdef COMPILE_NEW_KVS
	if(m_pShortSwitchDict)delete m_pShortSwitchDict;
	if(m_pLongSwitchDict)delete m_pLongSwitchDict;
#endif
}

void KviKvsSwitchList::clear()
{
#ifdef COMPILE_NEW_KVS
	if(m_pShortSwitchDict)
	{
		delete m_pShortSwitchDict;
		m_pShortSwitchDict = 0;
	}
	if(m_pLongSwitchDict)
	{
		delete m_pLongSwitchDict;
		m_pLongSwitchDict = 0;
	}
#endif
}

void KviKvsSwitchList::addShort(unsigned short uShortKey,KviKvsVariant * pVariant)
{
#ifdef COMPILE_NEW_KVS
	if(!m_pShortSwitchDict)
	{
		m_pShortSwitchDict = new QIntDict<KviKvsVariant>(11);
		m_pShortSwitchDict->setAutoDelete(true);
	}
	m_pShortSwitchDict->replace(uShortKey,pVariant);
#endif
}

void KviKvsSwitchList::addLong(const QString &szLongKey,KviKvsVariant * pVariant)
{
#ifdef COMPILE_NEW_KVS
	if(!m_pLongSwitchDict)
	{
		m_pLongSwitchDict = new QDict<KviKvsVariant>(11);
		m_pLongSwitchDict->setAutoDelete(true);
	}
	m_pLongSwitchDict->replace(szLongKey,pVariant);
#endif
}

