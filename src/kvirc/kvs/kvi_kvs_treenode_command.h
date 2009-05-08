#ifndef _KVI_KVS_TREENODE_COMMAND_H_
#define _KVI_KVS_TREENODE_COMMAND_H_
//=============================================================================
//
//   File : kvi_kvs_treenode_command.h
//   Creation date : Thu 09 Oct 2003 01:49:40 by Szymon Stefanek
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
#include "kvi_qstring.h"
#include "kvi_kvs_treenode_instruction.h"

class KviKvsParser;
class KviKvsTreeNodeSwitchList;

class KVIRC_API KviKvsTreeNodeCommand : public KviKvsTreeNodeInstruction
{
	// never instantiated
	friend class KviKvsParser;
public:
	KviKvsTreeNodeCommand(const QChar * pLocation,const QString &szCmdName);
	~KviKvsTreeNodeCommand();
protected:
	QString                           m_szCmdName; // command visible name
	KviKvsTreeNodeSwitchList        * m_pSwitches; // MAY BE 0!
public:
	virtual void contextDescription(QString &szBuffer);
	virtual void dump(const char * prefix);
	void dumpSwitchList(const char * prefix);
	const QString & commandName(){ return m_szCmdName; };
protected:
	void setSwitchList(KviKvsTreeNodeSwitchList * sw);
};


#endif //!_KVI_KVS_TREENODE_COMMAND_H_
