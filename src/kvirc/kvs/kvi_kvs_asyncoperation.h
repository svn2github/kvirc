#ifndef _KVI_KVS_ASYNCOPERATION_H_
#define _KVI_KVS_ASYNCOPERATION_H_
//=============================================================================
//
//   File : kvi_kvs_asyncoperation.h
//   Created on Sat 09 Jul 2005 04:54:24 by Szymon Stefanek
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
#include "kvi_list.h"
#include <qobject.h>

class KviWindow;

class KVIRC_API KviKvsAsyncOperation : public QObject
{
	Q_OBJECT
public:
	KviKvsAsyncOperation(KviWindow * pWnd);
	virtual ~KviKvsAsyncOperation();
protected:
	KviWindow * m_pWnd;
public:
	KviWindow * window(){ return m_pWnd; };
};


class KVIRC_API KviKvsAsyncOperationManager
{
	friend class KviKvsAsyncOperation;
public:
	KviKvsAsyncOperationManager();
	~KviKvsAsyncOperationManager();
protected:
	KviPtrList<KviKvsAsyncOperation> * m_pOperationList;
protected:
	void registerOperation(KviKvsAsyncOperation * o);
	void unregisterOperation(KviKvsAsyncOperation * o);
};

#endif //!_KVI_KVS_ASYNCOPERATION_H_
