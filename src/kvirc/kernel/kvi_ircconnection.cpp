//=============================================================================
//
//   File : kvi_ircconnection.cpp
//   Creation date : Mon 03 May 2004 01:45:42 by Szymon Stefanek
//
//   This file is part of the KVIrc IRC client distribution
//   Copyright (C) 2004-2008 Szymon Stefanek <pragma at kvirc dot net>
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



#include "kvi_ircconnection.h"
#include "kvi_ircconnectiontarget.h"
#include "kvi_ircconnectionuserinfo.h"
#include "kvi_ircconnectionserverinfo.h"
#include "kvi_ircconnectionstatedata.h"
#include "kvi_ircconnectionantictcpflooddata.h"
#include "kvi_ircconnectionnetsplitdetectordata.h"
#include "kvi_ircconnectionasyncwhoisdata.h"
#include "kvi_ircconnectionrequestqueue.h"
#include "kvi_ircconnectionstatistics.h"
#include "kvi_irclink.h"
#include "kvi_ircsocket.h"
#include "kvi_locale.h"
#include "kvi_ircserverdb.h"
#include "kvi_proxydb.h"
#include "kvi_error.h"
#include "kvi_out.h"
#include "kvi_options.h"
#include "kvi_console.h"
#include "kvi_netutils.h"
#include "kvi_internalcmd.h"
#include "kvi_frame.h"
#include "kvi_mexlinkfilter.h"
#include "kvi_malloc.h"
#include "kvi_memmove.h"
#include "kvi_debug.h"
#include "kvi_channel.h"
#include "kvi_query.h"
#include "kvi_app.h"
#include "kvi_databuffer.h"
#include "kvi_notifylist.h"
#include "kvi_dns.h"
#include "kvi_defaults.h"
#include "kvi_sparser.h"
#include "kvi_ircdatastreammonitor.h"
#include "kvi_databuffer.h"
#include "kvi_lagmeter.h"
#include "kvi_kvs_eventtriggers.h"
#include "kvi_kvs_script.h"
#include "kvi_mirccntrl.h"
#include "kvi_useridentity.h"
#include "kvi_identityprofile.h"

#include <QTimer>
#include <QTextCodec>

extern KVIRC_API KviServerDataBase   * g_pServerDataBase;
extern KVIRC_API KviProxyDataBase    * g_pProxyDataBase;

KviIrcConnection::KviIrcConnection(KviIrcContext * pContext,KviIrcConnectionTarget * pTarget,KviUserIdentity * pIdentity)
: QObject()
{
	m_bIdentdAttached = false;
	m_bInsideCapLsRequest = false;
	m_pContext = pContext;
	m_pConsole = pContext->console();
	m_pTarget = pTarget;
	m_pUserIdentity = pIdentity;
	m_pChannelList = new KviPointerList<KviChannel>;
	m_pChannelList->setAutoDelete(false);
	m_pQueryList = new KviPointerList<KviQuery>;
	m_pQueryList->setAutoDelete(false);
	m_pLink = new KviIrcLink(this);
	m_pUserDataBase = new KviIrcUserDataBase();
	m_pUserInfo = new KviIrcConnectionUserInfo();
	m_pServerInfo = new KviIrcConnectionServerInfo();
	m_pStateData = new KviIrcConnectionStateData();
	m_pAntiCtcpFloodData = new KviIrcConnectionAntiCtcpFloodData();
	m_pNetsplitDetectorData = new KviIrcConnectionNetsplitDetectorData();
	m_pAsyncWhoisData = new KviIrcConnectionAsyncWhoisData();
	m_pStatistics = new KviIrcConnectionStatistics();
	m_pNotifyListTimer = 0;
	m_pNotifyListManager = 0;
	m_pLocalhostDns = 0;
	m_pLagMeter = 0;
	m_eState = Idle;
	m_pRequestQueue = new KviRequestQueue();
	setupSrvCodec();
	setupTextCodec();
}

KviIrcConnection::~KviIrcConnection()
{
	if(m_bIdentdAttached)
		g_pFrame->executeInternalCommand(KVI_INTERNALCOMMAND_IDENT_STOP);

	m_bIdentdAttached = false;
	if(m_pLocalhostDns)
	{
		QObject::disconnect(m_pLocalhostDns,SIGNAL(lookupDone(KviDns *)),0,0);
		if(m_pLocalhostDns->isRunning())
		{
			m_pLocalhostDns->deleteLater();
		} else {
			delete m_pLocalhostDns;
		}
	}


	if(m_pNotifyListTimer)
	{
		delete m_pNotifyListTimer;
		m_pNotifyListTimer = 0;
	}

	if(m_pNotifyListManager)
	{
		delete m_pNotifyListManager; // destroy this before the userDb
		m_pNotifyListManager = 0;
	}

	if(m_pLagMeter)
	{
		delete m_pLagMeter;
		m_pLagMeter = 0;
	}

	delete m_pLink; // <-- this MAY trigger a linkTerminated() or something like this!
	delete m_pChannelList;
	delete m_pQueryList;
	delete m_pTarget;
	delete m_pUserDataBase;
	delete m_pUserInfo;
	delete m_pServerInfo;
	delete m_pStateData;
	delete m_pAntiCtcpFloodData;
	delete m_pNetsplitDetectorData;
	delete m_pAsyncWhoisData;
	delete m_pStatistics;
	delete m_pUserIdentity;
	delete m_pRequestQueue;
}

void KviIrcConnection::setEncoding(const QString & szEncoding)
{
	QTextCodec * c = KviLocale::codecForName(szEncoding.toLatin1());
	if(c == m_pTextCodec)
		return;
	if(!c)
	{
		m_pConsole->output(KVI_OUT_SYSTEMERROR,__tr2qs("Failed to set the encoding to %Q: mapping not available."),&szEncoding);
		return;
	}

	QString szTmp = c->name();
	for(KviChannel * ch = m_pChannelList->first(); ch; ch = m_pChannelList->next())
	{
		if((ch->textCodec() != c) && (ch->textCodec() != ch->defaultTextCodec())) // actually not using the default!
		{
			ch->forceTextCodec(c);
			if(_OUTPUT_VERBOSE)
				ch->output(KVI_OUT_VERBOSE,__tr2qs("Changed text encoding to %Q"),&szTmp);
		}
	}

	for(KviQuery * q = m_pQueryList->first(); q; q = m_pQueryList->next())
	{
		if((q->textCodec() != c) && (q->textCodec() != q->defaultTextCodec())) // actually not using the default!
		{
			q->forceTextCodec(c);
			if(_OUTPUT_VERBOSE)
				q->output(KVI_OUT_VERBOSE,__tr2qs("Changed text encoding to %Q"),&szTmp);
		}
	}
	m_pTextCodec = c;
	m_pConsole->setTextEncoding(szEncoding);
}

void KviIrcConnection::setupSrvCodec()
{
	// grab the codec: first look it up in the server data
	m_pSrvCodec = 0;
	if(!m_pTarget->server()->encoding().isEmpty())
	{
		m_pSrvCodec = KviLocale::codecForName(m_pTarget->server()->encoding().toLatin1());
		if(!m_pSrvCodec)
			qDebug("KviIrcConnection: can't find QTextCodec for encoding %s",m_pTarget->server()->encoding().toUtf8().data());
	}

	if(!m_pSrvCodec)
	{
		// try the network
		if(!m_pTarget->network()->encoding().isEmpty())
		{
			m_pSrvCodec = KviLocale::codecForName(m_pTarget->network()->encoding().toLatin1());
			if(!m_pSrvCodec)
				qDebug("KviIrcConnection: can't find QTextCodec for encoding %s",m_pTarget->network()->encoding().toUtf8().data());
		}
	}

	if(!m_pSrvCodec)
	{
		m_pSrvCodec = KviApp::defaultSrvCodec();
	}
	m_pConsole->setTextEncoding(QString(m_pSrvCodec->name()));
}

void KviIrcConnection::setupTextCodec()
{
	// grab the codec: first look it up in the server data
	m_pTextCodec = 0;
	if(!m_pTarget->server()->textEncoding().isEmpty())
	{
		m_pTextCodec = KviLocale::codecForName(m_pTarget->server()->textEncoding().toLatin1());
		if(!m_pTextCodec)qDebug("KviIrcConnection: can't find QTextCodec for encoding %s",m_pTarget->server()->textEncoding().toUtf8().data());
	}

	if(!m_pTextCodec)
	{
		// try the network
		if(!m_pTarget->network()->textEncoding().isEmpty())
		{
			m_pTextCodec = KviLocale::codecForName(m_pTarget->network()->textEncoding().toLatin1());
			if(!m_pTextCodec)qDebug("KviIrcConnection: can't find QTextCodec for encoding %s",m_pTarget->network()->textEncoding().toUtf8().data());
		}
	}

	if(!m_pTextCodec)
	{
		m_pTextCodec = KviApp::defaultTextCodec();
	}
}

/*
 * We're asking the connection itself to encode/decode text: use server specific encoding,
 * instead of the "user text" encoding used in channels
 */
QByteArray KviIrcConnection::encodeText(const QString & szText)
{
	if(!m_pSrvCodec)
		return szText.toUtf8();
	return m_pSrvCodec->fromUnicode(szText);
}

QString KviIrcConnection::decodeText(const char * pcText)
{
	if(!m_pSrvCodec)
		return QString(pcText);
	return m_pSrvCodec->toUnicode(pcText);
}

void KviIrcConnection::serverInfoReceived(const QString & szServerName, const QString & szUserModes, const QString & szChanModes)
{
	serverInfo()->setName(szServerName);
	serverInfo()->setSupportedUserModes(szUserModes);
	serverInfo()->setSupportedChannelModes(szChanModes);
	m_pConsole->updateCaption(); // for server name
	m_pConsole->frame()->childConnectionServerInfoChange(this);
}

const QString & KviIrcConnection::networkName()
{
	return m_pTarget->networkName();
}

void KviIrcConnection::abort()
{
	// this WILL trigger linkAttemptFailed() or linkTerminated()
	m_pLink->abort();
}

void KviIrcConnection::start()
{
	m_eState = Connecting;
	if(KVI_OPTION_BOOL(KviOption_boolUseIdentService) && KVI_OPTION_BOOL(KviOption_boolUseIdentServiceOnlyOnConnect))
	{
		g_pFrame->executeInternalCommand(KVI_INTERNALCOMMAND_IDENT_START);
		m_bIdentdAttached=true;
	}
	m_pLink->start();
}

void KviIrcConnection::linkEstabilished()
{
	m_eState = Connected;

	// setup reasonable defaults before notifying anyone
	m_pStatistics->setConnectionStartTime(kvi_unixTime());
	m_pStatistics->setLastMessageTime(kvi_unixTime());
	m_pServerInfo->setName(target()->server()->m_szHostname);

	// FIXME: With STARTTLS this is called TWICE!
	if(KviPointerList<KviIrcDataStreamMonitor> * l = context()->monitorList())
	{
		for(KviIrcDataStreamMonitor *m =l->first();m;m =l->next())
			m->connectionInitiated();
	}

	// FIXME: With STARTTLS this is called TWICE!
	context()->connectionEstabilished();

	// Ok...we're loggin in now
	resolveLocalHost();

	// HACK: this is needed to avoid timeouts connecting
	// This MUST go as one network packet to avoid possible handshake problems
	// on slow connections.

	// And, probably, NICK hack is better then PING hack as soon as freenode
	// not answer to ping!
	m_bInsideCapLsRequest = true;
	sendFmtData("CAP LS\r\nPING :%s",target()->server()->hostName().data());
}

void KviIrcConnection::handleCapLs()
{
	qDebug("handleCapLs");
	m_bInsideCapLsRequest = false;
#ifdef COMPILE_SSL_SUPPORT
	if(
		KVI_OPTION_BOOL(KviOption_boolUseStartTlsIfAvailable) &&
		(!link()->socket()->usingSSL()) &&
		(serverInfo()->supportedCaps().contains("tls",Qt::CaseInsensitive) ||
		target()->server()->supportsSTARTTLS())
	)
	{
		trySTARTTLS();
		return;
	}
#endif

	sendFmtData("CAP END");
	loginToIrcServer();
}

void KviIrcConnection::handleFailedCapLs()
{
	qDebug("handleFailedCapLs");
	m_bInsideCapLsRequest = false;
	loginToIrcServer();
}

void KviIrcConnection::linkTerminated()
{
	if(m_bIdentdAttached)
	{
		g_pFrame->executeInternalCommand(KVI_INTERNALCOMMAND_IDENT_STOP);
		m_bIdentdAttached=false;
	}
	m_eState = Idle;

	if(m_pNotifyListManager)
	{
		delete m_pNotifyListManager;
		m_pNotifyListManager = 0;
	}

	if(m_pLagMeter)
	{
		delete m_pLagMeter;
		m_pLagMeter = 0;
	}

	if(KviPointerList<KviIrcDataStreamMonitor> * l = context()->monitorList())
	{
		for(KviIrcDataStreamMonitor *m =l->first();m;m =l->next())
			m->connectionTerminated();
	}

	// Prepare data for an eventual reconnect
	context()->connectionTerminated();
}

void KviIrcConnection::linkAttemptFailed(int iError)
{
	if(m_bIdentdAttached)
	{
		g_pFrame->executeInternalCommand(KVI_INTERNALCOMMAND_IDENT_STOP);
		m_bIdentdAttached=false;
	}
	m_eState = Idle;
	context()->connectionFailed(iError);
}

KviChannel * KviIrcConnection::findChannel(const QString & szName)
{
	for(KviChannel * c = m_pChannelList->first(); c; c = m_pChannelList->next())
	{
		if(KviQString::equalCI(szName,c->windowName()))
			return c;
	}
	return 0;
}

int KviIrcConnection::getCommonChannels(const QString & szNick, QString & szChansBuffer, bool bAddEscapeSequences)
{
	int iCount = 0;
	for(KviChannel * c = m_pChannelList->first(); c; c = m_pChannelList->next())
	{
		if(c->isOn(szNick))
		{
			if(!szChansBuffer.isEmpty())
				szChansBuffer.append(", ");

			char uFlag = c->getUserFlag(szNick);
			if(uFlag)
			{
				KviQString::appendFormatted(szChansBuffer,bAddEscapeSequences ? "%c\r!c\r%Q\r" : "%c%Q",uFlag,&(c->windowName()));
			} else {
				if(bAddEscapeSequences)
					KviQString::appendFormatted(szChansBuffer,"\r!c\r%Q\r",&(c->windowName()));
				else szChansBuffer.append(c->windowName());
			}
			iCount++;
		}
	}
	return iCount;
}

void KviIrcConnection::unhighlightAllChannels()
{
	for(KviChannel * c = m_pChannelList->first(); c; c = m_pChannelList->next())
		c->unhighlight();
}

void KviIrcConnection::unhighlightAllQueries()
{
	for(KviQuery * c = m_pQueryList->first(); c; c = m_pQueryList->next())
		c->unhighlight();
}

void KviIrcConnection::partAllChannels()
{
	for(KviChannel * c = m_pChannelList->first(); c; c = m_pChannelList->next())
	{
		c->close();
		QApplication::processEvents(QEventLoop::ExcludeSocketNotifiers & QEventLoop::ExcludeUserInputEvents);
	}
}

void KviIrcConnection::closeAllChannels()
{
	while(m_pChannelList->first())
	{
		m_pChannelList->first()->close();
		QApplication::processEvents(QEventLoop::ExcludeSocketNotifiers & QEventLoop::ExcludeUserInputEvents);
	}
}

void KviIrcConnection::closeAllQueries()
{
	while(m_pQueryList->first())
	{
		m_pQueryList->first()->close();
		QApplication::processEvents(QEventLoop::ExcludeSocketNotifiers & QEventLoop::ExcludeUserInputEvents);
	}
}

KviChannel * KviIrcConnection::createChannel(const QString & szName)
{
	KviChannel * c = m_pContext->findDeadChannel(szName);
	if(c)
	{
		c->setAliveChan();
		if(!KVI_OPTION_BOOL(KviOption_boolCreateMinimizedChannels))
		{
			c->raise();
			c->setFocus();
		}
	} else {
		c = new KviChannel(m_pConsole->frame(),m_pConsole,szName);
		m_pConsole->frame()->addWindow(c,!KVI_OPTION_BOOL(KviOption_boolCreateMinimizedChannels));
	}
	return c;
}

KviQuery * KviIrcConnection::createQuery(const QString & szNick)
{
	KviQuery * q = m_pContext->findDeadQuery(szNick);
	if(!q)
	{
		q = findQuery(szNick);
		if(q)
			return q; // hm ?
	}
	if(q)
	{
		q->setAliveQuery();
		if(!KVI_OPTION_BOOL(KviOption_boolCreateMinimizedQuery))
		{
			q->raise();
			q->setFocus();
		}
	} else {
		q = new KviQuery(m_pConsole->frame(),m_pConsole,szNick);
		m_pConsole->frame()->addWindow(q,!KVI_OPTION_BOOL(KviOption_boolCreateMinimizedQuery));
	}
	return q;
}

KviQuery * KviIrcConnection::findQuery(const QString & szName)
{
	for(KviQuery * c = m_pQueryList->first(); c; c = m_pQueryList->next())
	{
		if(KviQString::equalCI(szName,c->windowName()))
			return c;
	}
	return 0;
}

void KviIrcConnection::registerChannel(KviChannel * c)
{
	m_pChannelList->append(c);
	if(KVI_OPTION_BOOL(KviOption_boolLogChannelHistory))
		g_pApp->addRecentChannel(c->windowName(),m_pTarget->networkName());
	emit(channelRegistered(c));
	emit(chanListChanged());
}

void KviIrcConnection::unregisterChannel(KviChannel * c)
{
	m_pChannelList->removeRef(c);
	requestQueue()->dequeueChannel(c);
	emit(channelUnregistered(c));
	emit(chanListChanged());
}

void KviIrcConnection::registerQuery(KviQuery * q)
{
	m_pQueryList->append(q);
}

void KviIrcConnection::unregisterQuery(KviQuery * q)
{
	if(m_pQueryList->removeRef(q))
		return;
}

void KviIrcConnection::keepChannelsOpenAfterDisconnect()
{
	while(KviChannel * c = m_pChannelList->first())
	{
		c->outputNoFmt(KVI_OUT_SOCKETERROR,__tr2qs("Connection to server lost"));
		c->setDeadChan();
	}
}

void KviIrcConnection::keepQueriesOpenAfterDisconnect()
{
	while(KviQuery * q = m_pQueryList->first())
	{
		q->outputNoFmt(KVI_OUT_SOCKETERROR,__tr2qs("Connection to server lost"));
		q->setDeadQuery();
	}
}

void KviIrcConnection::resurrectDeadQueries()
{
	while(KviQuery * q = m_pContext->firstDeadQuery())
	{
		q->outputNoFmt(KVI_OUT_SOCKETMESSAGE,__tr2qs("Connection to server established"));
		q->setAliveQuery();
	}
}


//=== Message send stuff ====================================================//
// Max buffer that can be sent to an IRC server is 512 bytes
// including CRLF. (ircd simply 'cuts' messages to 512 bytes
// and discards the remainig part)
// Note that 510 bytes of data is a reasonably long message :)
//
// 01234567890123456789012345678901234567890123456789
// 01234567890123456789012345678901234567890123456789
// 01234567890123456789012345678901234567890123456789
// 01234567890123456789012345678901234567890123456789
// 01234567890123456789012345678901234567890123456789
// 01234567890123456789012345678901234567890123456789
// 01234567890123456789012345678901234567890123456789
// 01234567890123456789012345678901234567890123456789
// 01234567890123456789012345678901234567890123456789
// 01234567890123456789012345678901234567890123456789
// 0123456789\r\n
//
// We keep a list of data to send , and flush it as soon as we can.
//

bool KviIrcConnection::sendFmtData(const char * pcFmt, ...)
{
	KviDataBuffer * pData = new KviDataBuffer(512);
	kvi_va_list(list);
	kvi_va_start(list,pcFmt);
	bool bTruncated;
	//sprintf the buffer up to 512 chars (adds a CRLF too)
	int iLen = kvi_irc_vsnprintf((char *)(pData->data()),pcFmt,list,&bTruncated);
	kvi_va_end(list);

	//adjust the buffer size
	if(iLen < 512)
		pData->resize(iLen);

	if(bTruncated)
	{
		if(!_OUTPUT_MUTE)
			m_pConsole->outputNoFmt(KVI_OUT_SOCKETWARNING,__tr2qs("[LINK WARNING]: Socket message truncated to 512 bytes."));
	}

	QString szMsg = QString::fromAscii((const char *)(pData->data()), iLen-2);

	// notify the monitors
	if(KviPointerList<KviIrcDataStreamMonitor> * l = context()->monitorList())
	{
		for(KviIrcDataStreamMonitor * m = l->first(); m; m = l->next())
		{
			if(m->outgoingMessage(szMsg.toUtf8().data()))
				return true;
		}
	}

	// Trigger OnOutboundTraffic event
	KviKvsVariantList * pParams = new KviKvsVariantList();
	pParams->append(szMsg);

	KVS_TRIGGER_EVENT(KviEvent_OnOutboundTraffic,m_pConsole->activeWindow(),pParams);

	return m_pLink->sendPacket(pData);
}

bool KviIrcConnection::sendData(const char * pcBuffer, int iBuflen)
{
	if(iBuflen < 0)
		iBuflen = (int)strlen(pcBuffer);
	if(iBuflen > 510)
	{
		iBuflen = 510;
		if(!_OUTPUT_MUTE)
			m_pConsole->outputNoFmt(KVI_OUT_SOCKETWARNING,__tr2qs("[LINK WARNING]: Socket message truncated to 512 bytes."));
	}

	KviDataBuffer * pData = new KviDataBuffer(iBuflen + 2);
	kvi_memmove(pData->data(),pcBuffer,iBuflen);
	*(pData->data() + iBuflen) = '\r';
	*(pData->data() + iBuflen + 1) = '\n';

	QString szMsg = (const char *)(pData->data());
	szMsg.truncate(iBuflen);

	// notify the monitors
	if(KviPointerList<KviIrcDataStreamMonitor> * l = context()->monitorList())
	{
		for(KviIrcDataStreamMonitor * m = l->first(); m; m = l->next())
		{
			if(m->outgoingMessage(szMsg.toUtf8().data()))
				return true;
		}
	}

	// Trigger OnOutboundTraffic event
	KviKvsVariantList * pParams = new KviKvsVariantList();
	pParams->append(szMsg);

	KVS_TRIGGER_EVENT(KviEvent_OnOutboundTraffic,m_pConsole->activeWindow(),pParams);

	return m_pLink->sendPacket(pData);
}

//==============================================================================================
// notify list management
//==============================================================================================

void KviIrcConnection::delayedStartNotifyList()
{
	__range_invalid(m_pNotifyListTimer);

	if(m_pNotifyListTimer)
		delete m_pNotifyListTimer;

	m_pNotifyListTimer = new QTimer();
	m_pNotifyListTimer->setInterval(15000);
	m_pNotifyListTimer->setSingleShot(true);
	m_pNotifyListTimer->start();
	connect(m_pNotifyListTimer,SIGNAL(timeout()),this,SLOT(restartNotifyList()));

	// This delay is large enough to fire after the MOTD has been sent,
	// even on the weirdest network.
	// If there is no MOTD, this timer will fire after 15 secs,
	// If there is a MOTD , restartNotifyList() will be triggered by RPL_ENDOFMOTD and
	// will kill the timer before it has fired.
}

void KviIrcConnection::endOfMotdReceived()
{
	// if the timer is still there running then just
	if(m_pNotifyListTimer)
		restartNotifyList();
}

void KviIrcConnection::restartNotifyList()
{
	if(m_pNotifyListTimer)
	{
		delete m_pNotifyListTimer;
		m_pNotifyListTimer = 0;
	}

	// clear it
	if(m_pNotifyListManager)
	{
		m_pNotifyListManager->stop(); // may need to remove watch entries
		delete m_pNotifyListManager;
		m_pNotifyListManager = 0;
	}

	if(!KVI_OPTION_BOOL(KviOption_boolUseNotifyList))
		return;

	if(serverInfo()->supportsWatchList() && KVI_OPTION_BOOL(KviOption_boolUseWatchListIfAvailable))
	{
		if(_OUTPUT_VERBOSE)
			m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("The server seems to support the WATCH notify list method, will try to use it"));
		m_pNotifyListManager = new KviWatchNotifyListManager(this);
	} else {
		if(KVI_OPTION_BOOL(KviOption_boolUseIntelligentNotifyListManager))
		{
			m_pNotifyListManager = new KviIsOnNotifyListManager(this);
		} else {
			m_pNotifyListManager = new KviStupidNotifyListManager(this);
		}
	}
	m_pNotifyListManager->start();
}

void KviIrcConnection::restartLagMeter()
{
	if(m_pLagMeter)
	{
		delete m_pLagMeter;
		m_pLagMeter = 0;
	}
	if(!KVI_OPTION_BOOL(KviOption_boolUseLagMeterEngine))
		return;
	m_pLagMeter = new KviLagMeter(this);
}

void KviIrcConnection::resolveLocalHost()
{
	QString szIp;

	if(!link()->socket()->getLocalHostIp(szIp,target()->server()->isIPv6()))
	{
		bool bGotIp = false;
		if(!KVI_OPTION_STRING(KviOption_stringLocalHostIp).isEmpty())
		{
#ifdef COMPILE_IPV6_SUPPORT
			if(target()->server()->isIPv6())
			{
				if(KviNetUtils::isValidStringIPv6(KVI_OPTION_STRING(KviOption_stringLocalHostIp)))
					bGotIp = true;
			} else {
#endif
				if(KviNetUtils::isValidStringIp(KVI_OPTION_STRING(KviOption_stringLocalHostIp)))
					bGotIp = true;
#ifdef COMPILE_IPV6_SUPPORT
			}
#endif
		}

		if(bGotIp)
		{
			m_pUserInfo->setLocalHostIp(KVI_OPTION_STRING(KviOption_stringLocalHostIp));
			if(!_OUTPUT_MUTE)
				m_pConsole->output(KVI_OUT_SYSTEMWARNING,__tr2qs("Can't resolve local host address, using user supplied one (%Q)"),
					&(m_pUserInfo->localHostIp()));

		} else {
			// FIXME : Maybe check for IPv6 here too ?
			m_pUserInfo->setLocalHostIp("127.0.0.1");
			if(!_OUTPUT_MUTE)
				m_pConsole->output(KVI_OUT_SYSTEMWARNING,__tr2qs("Can't resolve local host address, using default 127.0.0.1"),
					&(m_pUserInfo->localHostIp()));
		}
	} else {
		m_pUserInfo->setLocalHostIp(szIp);
		if(!_OUTPUT_QUIET)
			m_pConsole->output(KVI_OUT_SYSTEMMESSAGE,__tr2qs("Local host address is %Q"),
				&(m_pUserInfo->localHostIp()));
	}

	// For now this is the only we know
	m_pUserInfo->setHostName(m_pUserInfo->localHostIp());
	m_pUserInfo->setHostIp(m_pUserInfo->localHostIp());
}

void KviIrcConnection::changeAwayState(bool bAway)
{
	if(bAway)
		m_pUserInfo->setAway();
	else m_pUserInfo->setBack();

	m_pConsole->updateCaption();
	m_pConsole->frame()->childConnectionAwayStateChange(this);

	emit awayStateChanged();
}

void KviIrcConnection::userInfoReceived(const QString & szUserName, const QString & szHostName)
{
	userInfo()->setUserName(szUserName);
	QString szUnmaskedHost = m_pUserInfo->unmaskedHostName();
	// Update the user entry
	KviIrcUserEntry * e = userDataBase()->find(userInfo()->nickName());
	if(e) // should be there! (we have the permanent entry in the notify list view)
	{
		e->setUser(szUserName);
		if(!szHostName.isEmpty())e->setHost(szHostName);
	} // else buuug

	if(szHostName.isEmpty())
		return; // nothing to do anyway

	if(KviQString::equalCS(m_pUserInfo->hostName(),szHostName))
		return; // again nothing to do

	static bool warned_once = false;

	if(!warned_once)
	{
		if(!(m_pUserInfo->hostName().isEmpty() || KviQString::equalCS(m_pUserInfo->hostName(),m_pUserInfo->localHostIp())))
		{
			// ok, something weird is probably going on
			// is is non-empty and it is NOT the IP address we have set
			// at connection startup...
			// ...the server (or more likely the bouncer) must have changed his mind...
			if(!_OUTPUT_MUTE)
			{
				m_pConsole->output(KVI_OUT_SYSTEMWARNING,__tr2qs("The server seems to have changed the idea about the local hostname"));
				m_pConsole->output(KVI_OUT_SYSTEMWARNING,__tr2qs("You're probably using a broken bouncer or maybe something weird is happening on the IRC server"));
			}
			warned_once = true;
		}
	}

	// set it
	m_pUserInfo->setHostName(szHostName);

	bool bChangeIp = true;

	// if we don't have any routable IP yet, then it is worth to lookup the new hostname

#ifdef COMPILE_IPV6_SUPPORT
	if((KviNetUtils::isValidStringIp(m_pUserInfo->hostIp()) &&
		KviNetUtils::isRoutableIpString(m_pUserInfo->hostIp())) ||
		KviNetUtils::isValidStringIPv6(m_pUserInfo->hostIp()))
#else
	if((KviNetUtils::isValidStringIp(m_pUserInfo->hostIp()) &&
		KviNetUtils::isRoutableIpString(m_pUserInfo->hostIp())))
#endif
	{
		if(KVI_OPTION_BOOL(KviOption_boolDccGuessIpFromServerWhenLocalIsUnroutable) &&
			KVI_OPTION_BOOL(KviOption_boolDccBrokenBouncerHack))
		{
			if(!_OUTPUT_MUTE)
				m_pConsole->outputNoFmt(KVI_OUT_SYSTEMMESSAGE,__tr2qs("Here goes your \"broken bouncer hack\": The server has changed the hostname but I'll ignore the IP address change"));
			bChangeIp = false;
		}
	}

	if(bChangeIp)
	{
		// lookup the new hostname then...
#ifdef COMPILE_IPV6_SUPPORT
		if(KviNetUtils::isValidStringIp(szHostName) || KviNetUtils::isValidStringIPv6(szHostName))
#else
		if(KviNetUtils::isValidStringIp(szHostName))
#endif
		{
			if(!_OUTPUT_MUTE)
				m_pConsole->output(KVI_OUT_SYSTEMMESSAGE,__tr2qs("The local IP address as seen by the IRC server is %Q"),&szHostName);
			m_pUserInfo->setHostIp(szHostName);
		} else
#ifdef COMPILE_IPV6_SUPPORT
		if(KviNetUtils::isValidStringIp(szUnmaskedHost) || KviNetUtils::isValidStringIPv6(szUnmaskedHost))
#else
		if(KviNetUtils::isValidStringIp(szUnmaskedHost))
#endif
		{
			if(!_OUTPUT_MUTE)
				m_pConsole->output(KVI_OUT_SYSTEMMESSAGE,__tr2qs("The local IP address as seen by the IRC server is %Q"),&szUnmaskedHost);
			m_pUserInfo->setHostIp(szUnmaskedHost);

		} else {
			// look it up too
			if(m_pLocalhostDns)
				delete m_pLocalhostDns; // it could be only another local host lookup
			m_pLocalhostDns = new KviDns();
			connect(m_pLocalhostDns,SIGNAL(lookupDone(KviDns *)),this,SLOT(hostNameLookupTerminated(KviDns *)));

			if(!m_pLocalhostDns->lookup(szHostName,KviDns::Any))
			{
				if(!_OUTPUT_MUTE)
				{
					// don't change the string to aid the translators
					QString szTmp = __tr2qs("Can't start the DNS slave thread");
					m_pConsole->output(KVI_OUT_SYSTEMMESSAGE,__tr2qs("Unable to resolve the local hostname as seen by the IRC server: %Q"),&szTmp);
				}
				delete m_pLocalhostDns;
				m_pLocalhostDns = 0;
			} else {
				if(!_OUTPUT_MUTE)
					m_pConsole->output(KVI_OUT_SYSTEMMESSAGE,__tr2qs("Looking up the local hostname as seen by the IRC server (%Q)"),&szHostName);
			}
		}
	}
}

void KviIrcConnection::hostNameLookupTerminated(KviDns *)
{
	if(!m_pLocalhostDns)
	{
		qDebug("Something weird is happening: pDns != 0 but m_pLocalhostDns == 0 :/");
		return;
	}

	if(m_pLocalhostDns->state() != KviDns::Success)
	{
		QString szErr = KviError::getDescription(m_pLocalhostDns->error());
		if(!m_pUserInfo->hostIp().isEmpty())
			m_pConsole->output(KVI_OUT_SYSTEMMESSAGE,__tr2qs("Unable to resolve the local hostname as seen by the IRC server: %Q, using previously resolved %Q"),
			&szErr,&(m_pUserInfo->hostIp()));
		else
			m_pConsole->output(KVI_OUT_SYSTEMMESSAGE,__tr2qs("Unable to resolve the local hostname as seen by the IRC server: %Q"),
			&szErr);
	} else {
		QString szIpAddr = m_pLocalhostDns->firstIpAddress();
		m_pConsole->output(KVI_OUT_SYSTEMMESSAGE,__tr2qs("Local hostname as seen by the IRC server resolved to %Q"),&szIpAddr);
		m_pUserInfo->setHostIp(m_pLocalhostDns->firstIpAddress());
	}

	delete m_pLocalhostDns;
	m_pLocalhostDns = 0;
}

#ifdef COMPILE_SSL_SUPPORT
void KviIrcConnection::trySTARTTLS()
{
	// Check if the server supports STARTTLS protocol and we want to
	// connect through it
	debug("Sending STARTTLS command...");
	if(!sendFmtData("STARTTLS"))
	{
		// Cannot send command
		m_pConsole->output(KVI_OUT_SYSTEMMESSAGE,__tr2qs("Impossible to send STARTTLS command to the IRC server. Your connection will NOT be encrypted"));
		return;
	}
	m_pStateData->setSentStartTls();
}

void KviIrcConnection::enableStartTlsSupport(bool bEnable)
{
	if(bEnable)
	{
		// Ok, the server supports STARTTLS protocol
		// ssl handshake e switch del socket
		debug("Starting SSL handshake...");
		link()->socket()->enterSSLMode(); // FIXME: this should be forwarded through KviIrcLink, probably
	} else {
		// The server does not support STARTTLS
		m_pConsole->output(KVI_OUT_SYSTEMMESSAGE,__tr2qs("The server does not support STARTTLS command. Your connection will NOT be encrypted"));
	}
}
#endif // COMPILE_SSL_SUPPORT

void KviIrcConnection::useProfileData(KviIdentityProfileSet * pSet, const QString & szNetwork)
{
	KviIdentityProfile * pProfile = pSet->findNetwork(szNetwork);
	if(!pProfile) return;

	// Update connection data
	m_pUserInfo->setNickName(pProfile->nick());
	m_pUserInfo->setUserName(pProfile->userName());
	m_pUserInfo->setRealName(pProfile->realName());
}

void KviIrcConnection::loginToIrcServer()
{
	KviServer * pServer = target()->server();
	KviNetwork * pNet = target()->network();

	// Username
	pServer->m_szUser.trimmed();
	if(!pServer->m_szUser.isEmpty())
	{
		if(!_OUTPUT_MUTE)
			m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Using server specific username (%Q)"),&(pServer->m_szUser));
	} else {
		if(!pNet->userName().isEmpty())
		{
			if(!_OUTPUT_MUTE)
				m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Using network specific username (%Q)"),&(pNet->userName()));
			pServer->m_szUser = pNet->userName();
		} else {
			pServer->m_szUser = KVI_OPTION_STRING(KviOption_stringUsername);
		}
	}

	pServer->m_szUser.trimmed();
	if(pServer->m_szUser.isEmpty())
		pServer->m_szUser = KVI_DEFAULT_USERNAME;

	// For now this is the only we know
	m_pUserInfo->setUserName(pServer->m_szUser);
	m_pServerInfo->setName(pServer->m_szHostname);

	// Nick stuff
	pServer->m_szNick.trimmed();
	if(pServer->m_pReconnectInfo)
	{
		if(!_OUTPUT_MUTE)
			m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Using reconnect specific nickname (%Q)"),&(pServer->m_pReconnectInfo->m_szNick));
		m_pUserInfo->setNickName(pServer->m_pReconnectInfo->m_szNick);
		m_pStateData->setLoginNickIndex(0);
	}else if(!pServer->m_szNick.isEmpty())
	{
		if(!_OUTPUT_MUTE)
			m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Using server specific nickname (%Q)"),&(pServer->m_szNick));
		m_pUserInfo->setNickName(pServer->m_szNick);
		m_pStateData->setLoginNickIndex(0);
	} else {
		if(!pNet->nickName().isEmpty())
		{
			if(!_OUTPUT_MUTE)
				m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Using network specific nickname (%Q)"),&(pNet->nickName()));
			m_pUserInfo->setNickName(pNet->nickName());
			m_pStateData->setLoginNickIndex(0);
		} else {
			KVI_OPTION_STRING(KviOption_stringNickname1).trimmed();
			if(KVI_OPTION_STRING(KviOption_stringNickname1).isEmpty())
				KVI_OPTION_STRING(KviOption_stringNickname1) = KVI_DEFAULT_NICKNAME1;
			m_pUserInfo->setNickName(KVI_OPTION_STRING(KviOption_stringNickname1));
			m_pStateData->setLoginNickIndex(1);
		}
	}

	// Real name
	pServer->m_szRealName.trimmed();
	if(!pServer->m_szRealName.isEmpty())
	{
		if(!_OUTPUT_MUTE)
			m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Using server specific real name (%Q)"),&(pServer->m_szRealName));
		m_pUserInfo->setRealName(pServer->m_szRealName);
	} else {
		if(!pNet->realName().isEmpty())
		{
			if(!_OUTPUT_MUTE)
				m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Using network specific real name (%Q)"),&(pNet->realName()));
			m_pUserInfo->setRealName(pNet->realName());
		} else {
			m_pUserInfo->setRealName(KVI_OPTION_STRING(KviOption_stringRealname));
		}
	}

	// Check for identity profiles
	KviIdentityProfileSet * pSet = KviIdentityProfileSet::instance();
	bool bProfilesEnabled = pSet ? (pSet->isEnabled() && !pSet->isEmpty()) : false;
	if(bProfilesEnabled) useProfileData(pSet,pNet->name());

	// FIXME: The server's encoding!
	setupTextCodec();
	QByteArray szNick = encodeText(m_pUserInfo->nickName()); // never empty
	QByteArray szUser = encodeText(m_pUserInfo->userName()); // never empty
	QByteArray szReal = encodeText(m_pUserInfo->realName()); // may be empty

	if(!szReal.data())szReal = "";

	if(!_OUTPUT_MUTE)
		m_pConsole->output(KVI_OUT_SYSTEMMESSAGE,__tr2qs("Logging in as %Q!%Q :%Q"),
			&(m_pUserInfo->nickName()),&(m_pUserInfo->userName()),&(m_pUserInfo->realName()));

	// spity, 27.03.2005: follow the RFC2812 suggested order for connection registration
	// first the PASS, then NICK and then USER

	// The pass ?
	pServer->m_szPass.trimmed();
	if(!pServer->m_szPass.isEmpty())
	{
		QString szHidden = QString(pServer->m_szPass.length(), QChar('*'));

		if(!_OUTPUT_MUTE)
			m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Using server specific password (%s)"),szHidden.toUtf8().data());

		// The colon should allow user to use passwords with whitespaces.
		// Non-whitespace passwords are unaffected.
		if(!sendFmtData("PASS :%s",encodeText(pServer->m_szPass).data()))
		{
			// disconnected in the meantime
			return;
		}
	} else if(!pNet->password().isEmpty()) {
		QString szHidden = QString(pNet->password().length(), QChar('*'));

		if(!_OUTPUT_MUTE)
			m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Using network specific password (%s)"),szHidden.toUtf8().data());

		// The colon should allow user to use passwords with whitespaces.
		// Non-whitespace passwords are unaffected.
		if(!sendFmtData("PASS :%s",encodeText(pNet->password()).data()))
		{
			// disconnected in the meantime
			return;
		}
	}

	if(!sendFmtData("NICK %s",szNick.data()))
	{
		// disconnected :(
		return;
	}

	QString szGenderAvatarTag;
	int iGenderAvatarTag=0;
	if(KVI_OPTION_BOOL(KviOption_boolPrependGenderInfoToRealname) && !KVI_OPTION_STRING(KviOption_stringCtcpUserInfoGender).isEmpty())
	{
		if(KVI_OPTION_STRING(KviOption_stringCtcpUserInfoGender).startsWith("m",Qt::CaseInsensitive))
		{
			iGenderAvatarTag|=1;
		} else if(KVI_OPTION_STRING(KviOption_stringCtcpUserInfoGender).startsWith("f",Qt::CaseInsensitive))
		{
			iGenderAvatarTag|=2;
		}
	}

	if(KVI_OPTION_BOOL(KviOption_boolPrependAvatarInfoToRealname) && !KVI_OPTION_STRING(KviOption_stringMyAvatar).isEmpty())
	{
		iGenderAvatarTag|=4;
	}

	if(iGenderAvatarTag!=0)
	{
		szGenderAvatarTag.sprintf("%c%d%c",KVI_TEXT_COLOR,iGenderAvatarTag,KVI_TEXT_RESET);
		szReal.prepend(KviQString::toUtf8(szGenderAvatarTag));
	}

	if(!sendFmtData("USER %s 0 %s :%s",szUser.data(),
			KviQString::toUtf8(pServer->m_szHostname).data(),szReal.data()))
	{
		// disconnected in the meantime!
		return;
	}

	// permanent info in the user database
	m_pConsole->notifyListView()->join(m_pUserInfo->nickName(),"*","*");

	// set own avatar if we have it
	KviIrcUserEntry * e = userDataBase()->find(userInfo()->nickName());
	if(e) // should be there!
	{
		if(!e->avatar())
		{
			KviAvatar * av = m_pConsole->defaultAvatarFromOptions();
			if(av)
			{
				e->setAvatar(av);
				m_pConsole->notifyListView()->avatarChanged(userInfo()->nickName());
			}
		}
	} // else buuug

	if(KVI_OPTION_STRING(KviOption_stringCtcpUserInfoGender).startsWith("m",Qt::CaseInsensitive)){
			e->setGender(KviIrcUserEntry::Male);
	} else if(KVI_OPTION_STRING(KviOption_stringCtcpUserInfoGender).startsWith("f",Qt::CaseInsensitive)){
			e->setGender(KviIrcUserEntry::Female);
	}

	// on connect stuff ?

	QString szTmp = pNet->onConnectCommand();
	szTmp.trimmed();
	if(!szTmp.isEmpty())
	{
		if(_OUTPUT_VERBOSE)
			m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Executing scheduled network specific \"on connect\" commands"));
		KviKvsScript::run(szTmp,m_pConsole);
	}

	szTmp = pServer->onConnectCommand();
	szTmp.trimmed();
	if(!szTmp.isEmpty())
	{
		if(_OUTPUT_VERBOSE)
			m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Executing scheduled server specific \"on connect\" commands"));
		KviKvsScript::run(szTmp,m_pConsole);
	}

	szTmp = m_pUserIdentity->onConnectCommand();
	szTmp.trimmed();
	if(!szTmp.isEmpty())
	{
		if(_OUTPUT_VERBOSE)
			m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Executing scheduled identity specific \"on connect\" commands"));
		KviKvsScript::run(szTmp,m_pConsole);
	}

	// and wait for the server to agree...
}

void KviIrcConnection::nickChange(const QString & szNewNick)
{
	// FIXME: should the new nickname be decoded in some way ?
	m_pConsole->notifyListView()->nickChange(m_pUserInfo->nickName(),szNewNick);
	m_pUserInfo->setNickName(szNewNick);
	m_pConsole->output(KVI_OUT_NICK,__tr2qs("You have changed your nickname to %Q"),&szNewNick);
	m_pConsole->updateCaption();
	m_pConsole->frame()->childConnectionNickNameChange(this);
	emit nickNameChanged();
	g_pApp->addRecentNickname(szNewNick);
}

bool KviIrcConnection::changeUserMode(char cMode, bool bSet)
{
	__range_valid(m_pConnectionInfo);
	if(bSet)
	{
		if(m_pUserInfo->hasUserMode(cMode))
			return false;
		m_pUserInfo->addUserMode(cMode);
	} else {
		if(!m_pUserInfo->hasUserMode(cMode))
			return false;
		m_pUserInfo->removeUserMode(cMode);
	}
	m_pConsole->updateCaption();
	console()->frame()->childConnectionUserModeChange(this);
	emit userModeChanged();
	return true;
}

void KviIrcConnection::loginComplete(const QString & szNickName)
{
	if(context()->state() == KviIrcContext::Connected)
		return;

	context()->loginComplete();

	if(m_bIdentdAttached)
	{
		g_pFrame->executeInternalCommand(KVI_INTERNALCOMMAND_IDENT_STOP);
		m_bIdentdAttached = false;
	}

	if(szNickName != m_pUserInfo->nickName())
	{
		m_pConsole->output(KVI_OUT_SYSTEMMESSAGE,__tr2qs("The server refused the suggested nickname (%s) and named you %s instead"),m_pUserInfo->nickName().toUtf8().data(),szNickName.toUtf8().data());
		m_pConsole->notifyListView()->nickChange(m_pUserInfo->nickName(),szNickName);
		m_pUserInfo->setNickName(szNickName);
	}

	g_pApp->addRecentNickname(szNickName);

	bool bHaltOutput = false;
	bHaltOutput = KVS_TRIGGER_EVENT_0_HALTED(KviEvent_OnIRC,m_pConsole);

	if(!bHaltOutput)
		m_pConsole->outputNoFmt(KVI_OUT_IRC,__tr2qs("Login operations complete, happy ircing!"));

	resurrectDeadQueries();

	// on connect stuff ?
	QString szTmp = target()->network()->onLoginCommand();
	szTmp.trimmed();
	if(!szTmp.isEmpty())
	{
		if(_OUTPUT_VERBOSE)
			m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Executing scheduled network specific \"on login\" commands"));
		KviKvsScript::run(szTmp,m_pConsole);
	}

	szTmp = target()->server()->onLoginCommand();
	szTmp.trimmed();
	if(!szTmp.isEmpty())
	{
		if(_OUTPUT_VERBOSE)
			m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Executing scheduled server specific \"on login\" commands"));
		KviKvsScript::run(szTmp,m_pConsole);
	}

	szTmp = m_pUserIdentity->onLoginCommand();
	szTmp.trimmed();
	if(!szTmp.isEmpty())
	{
		if(_OUTPUT_VERBOSE)
			m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Executing scheduled identity specific \"on login\" commands"));
		KviKvsScript::run(szTmp,m_pConsole);
	}

	// Set the configured umode
	QString szModeStr = target()->server()->initUMode();

	if(szModeStr.isEmpty())
		szModeStr = KVI_OPTION_STRING(KviOption_stringDefaultUserMode);

	if(_OUTPUT_VERBOSE)
		m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Setting configured user mode"));
	sendFmtData("MODE %s +%s",encodeText(m_pUserInfo->nickName()).data(),encodeText(szModeStr).data());

	delayedStartNotifyList();
	restartLagMeter();

	if(KVI_OPTION_BOOL(KviOption_boolShowChannelsJoinOnIrc))
		m_pConsole->frame()->executeInternalCommand(KVI_INTERNALCOMMAND_CHANNELSJOIN_OPEN);


	// join saved channels
	QString szChannels,szProtectedChannels,szPasswords,szCurPass,szCurChan;

	if(!(m_pStateData->commandToExecAfterConnect().isEmpty()))
	{
		KviStr tmp = m_pStateData->commandToExecAfterConnect();
		KviKvsScript::run(tmp.ptr(),m_pConsole);
	}

	if(target()->server()->m_pReconnectInfo)
	{
		if(!target()->server()->m_pReconnectInfo->m_szJoinChannels.isEmpty())
			sendFmtData("JOIN %s",encodeText(target()->server()->m_pReconnectInfo->m_szJoinChannels).data());

		KviQuery * pQuery;

		for(QStringList::Iterator it = target()->server()->m_pReconnectInfo->m_szOpenQueryes.begin();
			it != target()->server()->m_pReconnectInfo->m_szOpenQueryes.end();it++)
		{
			QString szNick = *it;
			pQuery = findQuery(szNick);
			if(!pQuery)
			{
				pQuery = createQuery(szNick);
				QString szUser;
				QString szHost;

				KviIrcUserDataBase * db = userDataBase();
				if(db)
				{
					KviIrcUserEntry * e = db->find(szNick);
					if(e)
					{
						szUser = e->user();
						szHost = e->host();
					}
				}
				pQuery->setTarget(szNick,szUser,szHost);
			}
			pQuery->autoRaise();
			pQuery->setFocus();
		}
		delete target()->server()->m_pReconnectInfo;
		target()->server()->m_pReconnectInfo=0;
	}else {
		if(target()->network()->autoJoinChannelList())
		{
			if(_OUTPUT_VERBOSE)
				m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Auto-joining network specific channels"));

			QStringList * l = target()->network()->autoJoinChannelList();
			if(l->count()!=0)
			{
				for(QStringList::Iterator it = l->begin(); it != l->end(); ++it)
				{
					szCurPass=(*it).section(':',1);
					if(szCurPass.isEmpty())
					{
						if(!szChannels.isEmpty())
							szChannels.append(",");
						szCurChan = (*it).section(':',0,0);
						if(!(szCurChan[0]=='#' || szCurChan[0]=='&' || szCurChan[0]=='!'))
							szCurChan.prepend('#');
						szChannels.append(szCurChan);
					} else {
						if(!szProtectedChannels.isEmpty())
							szProtectedChannels.append(",");
						szCurChan = (*it).section(':',0,0);
						if(!(szCurChan[0]=='#' || szCurChan[0]=='&' || szCurChan[0]=='!'))
							szCurChan.prepend('#');
						szProtectedChannels.append(szCurChan);
						if(!szPasswords.isEmpty())
							szPasswords.append(",");
						szPasswords.append(szCurPass);
					}
				}
			}
		}

		if(target()->server()->autoJoinChannelList())
		{
			if(_OUTPUT_VERBOSE)
				m_pConsole->output(KVI_OUT_VERBOSE,__tr2qs("Auto-joining server specific channels"));

			QStringList * l = target()->server()->autoJoinChannelList();
			if(l->count()!=0)
			{
				for(QStringList::Iterator it = l->begin(); it != l->end(); ++it)
				{
					szCurPass=(*it).section(':',1);
					if(szCurPass.isEmpty())
					{
						if(!szChannels.isEmpty())
							szChannels.append(",");
						szCurChan = (*it).section(':',0,0);
						if(!(szCurChan[0]=='#' || szCurChan[0]=='&' || szCurChan[0]=='!'))
							szCurChan.prepend(':');
						szChannels.append(szCurChan);
					} else {
						if(!szProtectedChannels.isEmpty())
							szProtectedChannels.append(",");
						szCurChan = (*it).section(':',0,0);
						if(!(szCurChan[0]=='#' || szCurChan[0]=='&' || szCurChan[0]=='!'))
							szCurChan.prepend('#');
						szProtectedChannels.append(szCurChan);
						if(!szPasswords.isEmpty())
							szPasswords.append(",");
						szPasswords.append(szCurPass);
					}
				}
			}
		}

		QString szCommand;
		if( (!szChannels.isEmpty()) || (!szProtectedChannels.isEmpty()) )
		{
			szCommand.append(szProtectedChannels);
			if(!szProtectedChannels.isEmpty() && !szChannels.isEmpty())
				szCommand.append(',');
			szCommand.append(szChannels);
			szCommand.append(" ");
			szCommand.append(szPasswords);

			sendFmtData("JOIN %s",encodeText(szCommand).data());
		}
	}
	// minimize after connect
	if(KVI_OPTION_BOOL(KviOption_boolMinimizeConsoleAfterConnect))
		m_pConsole->minimize();
}

void KviIrcConnection::incomingMessage(const char * pcMessage)
{
	// A message has arrived from the current server
	// First of all , notify the monitors
	if(KviPointerList<KviIrcDataStreamMonitor> * l = context()->monitorList())
	{
		for(KviIrcDataStreamMonitor * m = l->first(); m; m = l->next())
		{
			if(m->incomingMessage(pcMessage))
				return;
		}
	}
	// set the last message time
	m_pStatistics->setLastMessageTime(kvi_unixTime());
	// and pass it to the server parser for processing
	g_pServerParser->parseMessage(pcMessage,this);
}

void KviIrcConnection::incomingMessageNoFilter(const char * pcMessage)
{
	// set the last message time
	m_pStatistics->setLastMessageTime(kvi_unixTime());
	// and pass it to the server parser for processing
	g_pServerParser->parseMessage(pcMessage,this);
}

void KviIrcConnection::heartbeat(kvi_time_t tNow)
{
	if(m_eState == Connected)
	{
		if(KVI_OPTION_BOOL(KviOption_boolEnableAwayListUpdates))
		{
			// update the channel WHO lists (fixes users away state)
			// first of all, we send our request not more often than every 50 secs
			if((tNow - stateData()->lastSentChannelWhoRequest()) > 50)
			{
				// we also make sure that the last sent request is older than
				// the last received reply
				if(stateData()->lastSentChannelWhoRequest() <= stateData()->lastReceivedChannelWhoReply())
				{
					// find the channel that has the older list now
					kvi_time_t tOldest = tNow;
					KviChannel * pOldest = 0;
					for(KviChannel * pChan = m_pChannelList->first();pChan;pChan = m_pChannelList->next())
					{
						if(pChan->lastReceivedWhoReply() < tOldest)
						{
							pOldest = pChan;
							tOldest = pChan->lastReceivedWhoReply();
						}
					}
					// if the oldest chan who list is older than 150 secs, update it
					if((tNow - tOldest) > 150)
					{
						// ok, sent the request for this channel
						stateData()->setLastSentChannelWhoRequest(tNow);
						QString szChanName = encodeText(pOldest->windowName()).data();
						if(_OUTPUT_PARANOIC)
							console()->output(KVI_OUT_VERBOSE,__tr2qs("Updating away state for channel %Q"),&szChanName);
						if(lagMeter())
						{
							KviStr tmp(KviStr::Format,"WHO %s",encodeText(pOldest->windowName()).data());
							lagMeter()->lagCheckRegister(tmp.ptr(),70);
						}
						pOldest->setSentSyncWhoRequest();
						if(!sendFmtData("WHO %s",encodeText(pOldest->windowName()).data()))
							return;
					}
				}
			}
		}
	}
}
