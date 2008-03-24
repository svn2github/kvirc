//=============================================================================
//
//   File : kvi_kvs_treenode_constantdata.cpp
//   Created on Tue 07 Oct 2003 02:21:59 by Szymon Stefanek
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

#include "kvi_kvs_treenode_constantdata.h"


KviKvsTreeNodeConstantData::KviKvsTreeNodeConstantData(const QChar * pLocation,KviKvsVariant * v)
: KviKvsTreeNodeData(pLocation)
{
	m_pValue = v;
}


KviKvsTreeNodeConstantData::~KviKvsTreeNodeConstantData()
{
	delete m_pValue;
}

void KviKvsTreeNodeConstantData::contextDescription(QString &szBuffer)
{
	szBuffer = "Constant Data Evaluation";
}

bool KviKvsTreeNodeConstantData::convertStringConstantToNumeric()
{
	if(m_pValue->isString())
	{
		kvs_int_t iVal;
		if(m_pValue->asInteger(iVal))
		{
			m_pValue->setInteger(iVal);
			return true;
		}
		
		kvs_real_t dVal;
		if(m_pValue->asReal(dVal))
		{
			m_pValue->setReal(dVal);
			return true;
		}
	}
	return true;
}

void KviKvsTreeNodeConstantData::dump(const char * prefix)
{
	debug("%s ConstantData",prefix);
	QString tmp = prefix;
	tmp.prepend("  ");
	m_pValue->dump(tmp.utf8().data());
}

bool KviKvsTreeNodeConstantData::evaluateReadOnly(KviKvsRunTimeContext * c,KviKvsVariant * pBuffer)
{
	pBuffer->copyFrom(m_pValue);
	return true;
}
