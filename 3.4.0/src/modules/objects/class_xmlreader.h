#ifndef _CLASS_XMLREADER_H_
#define _CLASS_XMLREADER_H_
//=============================================================================
//
//   File : class_xmlreader.h
//   Created on Tue 27 Dec 2005 00:14:09 by Szymon Stefanek
//
//   This file is part of the KVIrc IRC Client distribution
//   Copyright (C) 2005 Szymon Stefanek <pragma at kvirc dot net>
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
#include "kvi_qstring.h"

#include "object_macros.h"

class KviKvsObject_xmlreader : public KviKvsObject
{
	Q_OBJECT
public:
	KVSO_DECLARE_OBJECT(KviKvsObject_xmlreader)
public:
	bool function_parse(KviKvsObjectFunctionCall *c);
	bool function_lastError(KviKvsObjectFunctionCall *c);
	
	void fatalError(const QString &szError);
protected:
	QString m_szLastError;	
};

#endif //!_CLASS_XMLREADER_H_
