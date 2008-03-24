//=============================================================================
//
//   File : kvi_kvs_treenode_thisobjectfunctioncall.cpp
//   Created on Tue 07 Oct 2003 03:28:10 by Szymon Stefanek
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

#include "kvi_kvs_treenode_thisobjectfunctioncall.h"
#include "kvi_kvs_object.h"


KviKvsTreeNodeThisObjectFunctionCall::KviKvsTreeNodeThisObjectFunctionCall(const QChar * pLocation,const QString &szFncName,KviKvsTreeNodeDataList * pParams)
: KviKvsTreeNodeObjectFunctionCall(pLocation,szFncName,pParams)
{
}

KviKvsTreeNodeThisObjectFunctionCall::~KviKvsTreeNodeThisObjectFunctionCall()
{
}

void KviKvsTreeNodeThisObjectFunctionCall::contextDescription(QString &szBuffer)
{
#ifdef COMPILE_NEW_KVS
	szBuffer = "\"This\" Object Function Call \"";
	szBuffer += m_szFunctionName;
	szBuffer += "\"";
#endif
}

void KviKvsTreeNodeThisObjectFunctionCall::dump(const char * prefix)
{
#ifdef COMPILE_NEW_KVS
	debug("%s ThisObjectFunctionCall(%s)",prefix,m_szFunctionName.utf8().data());
	QString tmp = prefix;
	tmp.append("  ");
	m_pParams->dump(tmp.utf8().data());
#endif
}


bool KviKvsTreeNodeThisObjectFunctionCall::evaluateReadOnlyInObjectScope(KviKvsObject * o,KviKvsRunTimeContext * c,KviKvsVariant * pBuffer)
{
#ifdef COMPILE_NEW_KVS
	KviKvsVariantList l;
	if(!m_pParams->evaluate(c,&l))return false;
	pBuffer->setNothing();
	c->setDefaultReportLocation(this);
	return o->callFunction(c->thisObject(),m_szFunctionName,QString::null,c,pBuffer,&l);
#else
	return false;
#endif
}


