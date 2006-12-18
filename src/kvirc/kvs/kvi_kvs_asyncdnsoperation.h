#ifndef _KVI_KVS_ASYNCDNSOPERATION_H_
#define _KVI_KVS_ASYNCDNSOPERATION_H_
//=============================================================================
//
//   File : kvi_kvs_asyncdnsoperation.h
//   Created on Sun 10 Jul 2005 04:36:15 by Szymon Stefanek
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
#include "kvi_kvs_asyncoperation.h"
#include "kvi_qstring.h"
#include "kvi_dns.h"

class KviWindow;
class KviKvsScript;
class KviKvsVariant;

class KVIRC_API KviKvsAsyncDnsOperation : public KviKvsAsyncOperation
{
	Q_OBJECT
public:
	KviKvsAsyncDnsOperation(KviWindow * pWnd,QString &szQuery,KviDns::QueryType eType,KviKvsScript * pCallback = 0,KviKvsVariant * pMagic = 0);
	virtual ~KviKvsAsyncDnsOperation();
protected:
	KviDns * m_pDns;
	KviDns::QueryType m_eType;
	KviKvsVariant * m_pMagic;
	KviKvsScript * m_pCallback;
	QString m_szQuery;
protected slots:
	void lookupTerminated(KviDns *);
	void dnsStartFailed();
};

#endif //!_KVI_KVS_ASYNCDNSOPERATION_H_
