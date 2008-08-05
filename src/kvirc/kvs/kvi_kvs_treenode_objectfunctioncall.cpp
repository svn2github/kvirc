//=============================================================================
//
//   File : kvi_kvs_treenode_objectfunctioncall.cpp
//   Created on Tue 07 Oct 2003 03:26:07 by Szymon Stefanek
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

#include "kvi_kvs_treenode_objectfunctioncall.h"

KviKvsTreeNodeObjectFunctionCall::KviKvsTreeNodeObjectFunctionCall(const QChar * pLocation,const QString &szFncName,KviKvsTreeNodeDataList * pParams)
: KviKvsTreeNodeFunctionCall(pLocation,szFncName,pParams)
{
}

KviKvsTreeNodeObjectFunctionCall::~KviKvsTreeNodeObjectFunctionCall()
{
}

void KviKvsTreeNodeObjectFunctionCall::contextDescription(QString &szBuffer)
{
	szBuffer = "Object Function Call \"";
	szBuffer += m_szFunctionName;
	szBuffer += "\"";
}

void KviKvsTreeNodeObjectFunctionCall::dump(const char * prefix)
{
	qDebug("%s ObjectFunctionCall(%s)",prefix,m_szFunctionName.toUtf8().data());
	QString tmp = prefix;
	tmp.append("  ");
	m_pParams->dump(tmp.toUtf8().data());
}

bool KviKvsTreeNodeObjectFunctionCall::canEvaluateInObjectScope()
{
	return true;
}
