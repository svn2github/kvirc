#ifndef _KVI_KVS_PROCESSMANAGER_H_
#define _KVI_KVS_PROCESSMANAGER_H_
//=============================================================================
//
//   File : kvi_kvs_processmanager.h
//   Created on Wed 07 Apr 2004 03:03:52 by Szymon Stefanek
//
//   This file is part of the KVIrc IRC client distribution
//   Copyright (C) 2004-2005 Szymon Stefanek <pragma at kvirc dot net>
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
#include "kvi_window.h"
#include "kvi_kvs_variant.h"
#include "kvi_kvs_runtimecontext.h"
#include "kvi_kvs_variant.h"

#include "kvi_pointerhashtable.h"

#ifdef Unsorted
	#undef Unsorted
#endif

#include "kvi_process.h"

#include <QTimer>

#define KVI_KVS_PROCESSDESCRIPTOR_TRIGGERSTDOUT 1
#define KVI_KVS_PROCESSDESCRIPTOR_TRIGGERSTDERR 2
#define KVI_KVS_PROCESSDESCRIPTOR_TRIGGERTERMINATED 4
#define KVI_KVS_PROCESSDESCRIPTOR_OUTPUTBYBLOCKS 8
#define KVI_KVS_PROCESSDESCRIPTOR_TRIGGERSTARTED 16
#define KVI_KVS_PROCESSDESCRIPTOR_NOSHELL 32
#define KVI_KVS_PROCESSDESCRIPTOR_KILLIFNOWINDOW 64


class KviWindow;
class KviKvsScript;
class KviKvsVariant;

typedef struct _KviKvsProcessDescriptorData
{
	QString         szShell;
	QString         szCommandline;
	KviKvsScript  * pCallback;
	KviKvsVariant * pMagic;
	KviWindow     * pWnd;
	int             iFlags;
	int             iMaxRunTime;  // 0 for no timeout
	int             iPingTimeout; // 0 for no ping timeout
} KviKvsProcessDescriptorData;

class KviKvsProcessManager;

class KVIRC_API KviKvsProcessAsyncOperation : public KviKvsAsyncOperation
{
	friend class KviKvsProcessManager;
	Q_OBJECT
public:
	KviKvsProcessAsyncOperation(KviKvsProcessDescriptorData * d);
	~KviKvsProcessAsyncOperation();
protected:
	enum CallbackEvent { EventStarted,EventTerminated,EventStdout,EventStderr,EventPing };
private:
	KviProcess                  * m_pProcess;
	KviKvsExtendedRunTimeData   * m_pExtendedRunTimeData;
	QString                       m_szStdoutBuffer;
	QString                       m_szStderrBuffer;
	KviKvsProcessDescriptorData * m_pData;
	QTimer                      * m_pPingTimer;
	QTimer                      * m_pRunTimeTimer;
	bool                          m_bDeletePending;
public:
	bool start();
private slots:
	void maxRunTimeExpired();
	void ping();
	void readStdout();
	void readStderr();
	void processExited(int exitCode);
	void processStarted();
	void selfDelete();
private:
	bool trigger(CallbackEvent e,const QString &szData);
	void triggerSelfDelete();
};


#endif //!_KVI_KVS_PROCESSMANAGER_H_
