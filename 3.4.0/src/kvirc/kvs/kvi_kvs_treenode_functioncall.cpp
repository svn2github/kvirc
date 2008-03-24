//=============================================================================
//
//   File : kvi_kvs_treenode_functioncall.cpp
//   Created on Tue 07 Oct 2003 03:14:44 by Szymon Stefanek
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

#include "kvi_kvs_treenode_functioncall.h"


KviKvsTreeNodeFunctionCall::KviKvsTreeNodeFunctionCall(const QChar * pLocation,const QString &szFunctionName,KviKvsTreeNodeDataList * pParams)
: KviKvsTreeNodeData(pLocation)
{
	m_szFunctionName = szFunctionName;
	m_pParams = pParams;
	m_pParams->setParent(this);
}

KviKvsTreeNodeFunctionCall::~KviKvsTreeNodeFunctionCall()
{
	delete m_pParams;
}

void KviKvsTreeNodeFunctionCall::contextDescription(QString &szBuffer)
{
	szBuffer = "Function Call";
}

void KviKvsTreeNodeFunctionCall::dump(const char * prefix)
{
	debug("%s FunctionCall",prefix);
}

bool KviKvsTreeNodeFunctionCall::canEvaluateToObjectReference()
{
	return true;
}

bool KviKvsTreeNodeFunctionCall::isFunctionCall()
{
	return true;
}

