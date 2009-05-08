#ifndef _KVI_KVS_TREENODE_FUNCTIONCALL_H_
#define _KVI_KVS_TREENODE_FUNCTIONCALL_H_
//=============================================================================
//
//   File : kvi_kvs_treenode_functioncall.h
//   Creation date : Tue 07 Oct 2003 03:14:44 by Szymon Stefanek
//
//   This file is part of the KVIrc IRC client distribution
//   Copyright (C) 2003-2008 Szymon Stefanek <pragma at kvirc dot net>
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
#include "kvi_kvs_treenode_data.h"
#include "kvi_kvs_treenode_datalist.h"

class KVIRC_API KviKvsTreeNodeFunctionCall : public KviKvsTreeNodeData
{
public:
	KviKvsTreeNodeFunctionCall(const QChar * pLocation,const QString &szFunctionName,KviKvsTreeNodeDataList * pParams);
	~KviKvsTreeNodeFunctionCall();
protected:
	QString m_szFunctionName;
	KviKvsTreeNodeDataList * m_pParams; // never 0
public:
	virtual void contextDescription(QString &szBuffer);
	virtual void dump(const char * prefix);
	virtual bool canEvaluateToObjectReference(); // yes
	virtual bool isFunctionCall(); // yes
};


#endif //!_KVI_KVS_TREENODE_FUNCTIONCALL_H_
