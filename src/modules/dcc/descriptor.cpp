//=============================================================================
//
//   File : descriptor.cpp
//   Creation date : Tue Jul 23 01:11:54 2002 GMT by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2002-2008 Szymon Stefanek (pragma at kvirc dot net)
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

#include "descriptor.h"
#include "send.h"
#include "window.h"

#include "kvi_locale.h"
#include "kvi_kvs_eventtriggers.h"
#include "kvi_window.h"
#include "kvi_app.h"

static unsigned int g_uNextDescriptorId = 1; // we use 0 as an invalid descriptor id
static KviPointerHashTable<int,KviDccDescriptor> * g_pDescriptorDict = 0;

KviPointerHashTable<int,KviDccDescriptor> * KviDccDescriptor::descriptorDict()
{
	return g_pDescriptorDict;
}

KviDccDescriptor::KviDccDescriptor(const KviDccDescriptor & src)
{
	copyFrom(src);
}

KviDccDescriptor::KviDccDescriptor(KviConsole * pConsole)
{
	m_pConsole   = pConsole;
	m_pDccWindow = 0;
	m_pDccTransfer = 0;

	m_uId = g_uNextDescriptorId;
	g_uNextDescriptorId++;

	m_szId.setNum(m_uId);

	if(!g_pDescriptorDict)
	{
		g_pDescriptorDict = new KviPointerHashTable<int,KviDccDescriptor>;
		g_pDescriptorDict->setAutoDelete(false);
	}
	g_pDescriptorDict->replace((long)m_uId,this);

	szNick       = __tr_ctx("unknown","dcc");
	szUser       = szNick;
	szHost       = szNick;

	szLocalNick  = szNick;
	szLocalUser  = szNick;
	szLocalHost  = szNick;

	szIp         = szNick;
	szPort       = szNick;


	bSendRequest      = true;
	bDoTimeout        = true;
	bIsTdcc           = false;
	bOverrideMinimize = false;
	bShowMinimized    = false;
	bAutoAccept       = false;
#ifdef COMPILE_SSL_SUPPORT
	bIsSSL            = false;
#endif
	bRecvFile         = false;
	bResume           = false;
	bNoAcks           = false;
	bIsIncomingAvatar = false;

	iSampleRate       = 0;

	m_bCreationEventTriggered = false;
}

KviDccDescriptor::~KviDccDescriptor()
{
	if(m_bCreationEventTriggered)
	{
		KviWindow * pEventWindow = m_pConsole;
		if(!pEventWindow)pEventWindow = g_pApp->activeConsole(); // any console
		else {
			if(!(g_pApp->windowExists(pEventWindow)))pEventWindow = g_pApp->activeConsole();
		}

		if(pEventWindow)
		{
			// recheck it again...
			if(g_pApp->windowExists(pEventWindow))
			{
				KVS_TRIGGER_EVENT_1(KviEvent_OnDCCSessionDestroyed,pEventWindow,m_szId);
			}
		}
	}

	if(g_pDescriptorDict)
	{
		g_pDescriptorDict->remove((long)m_uId);
		if(g_pDescriptorDict->count() < 1)
		{
			delete g_pDescriptorDict;
			g_pDescriptorDict = 0;
		}
	}

}

void KviDccDescriptor::triggerCreationEvent()
{
	if(m_bCreationEventTriggered)
	{
		debug("Ops.. trying to trigger OnDccSessionCreated twice");
		return;
	}
	m_bCreationEventTriggered = true;
	KviWindow * pEventWindow = m_pConsole;
	if(!pEventWindow)pEventWindow = g_pApp->activeConsole(); // any console
	if(pEventWindow)
	{
		KVS_TRIGGER_EVENT_1(KviEvent_OnDCCSessionCreated,pEventWindow,m_szId);
	}
}

KviDccDescriptor * KviDccDescriptor::find(unsigned int uId)
{
	if(!g_pDescriptorDict)return 0;
	return g_pDescriptorDict->find((long)uId);
}

void KviDccDescriptor::copyFrom(const KviDccDescriptor &src)
{
	m_uId = g_uNextDescriptorId;
	g_uNextDescriptorId++;
	m_szId.setNum(m_uId);

	if(!g_pDescriptorDict)
	{
		g_pDescriptorDict = new KviPointerHashTable<int,KviDccDescriptor>;
		g_pDescriptorDict->setAutoDelete(false);
	}
	g_pDescriptorDict->replace((long)m_uId,this);
	m_bCreationEventTriggered=false;

	m_pDccWindow          = src.m_pDccWindow;
	m_pDccTransfer        = src.m_pDccTransfer;
	szType                = src.szType;
	szNick                = src.szNick;
	szUser                = src.szUser;
	szHost                = src.szHost;
	szLocalNick           = src.szLocalNick;
	szLocalUser           = src.szLocalUser;
	szLocalHost           = src.szLocalHost;
	szIp                  = src.szIp;
	szPort                = src.szPort;
	m_pConsole            = src.console();
	m_szZeroPortRequestTag= src.zeroPortRequestTag();
	bActive               = src.bActive;
	szListenIp            = src.szListenIp;
	szListenPort          = src.szListenPort;
	szFakeIp              = src.szFakeIp;
	szFakePort            = src.szFakePort;
	bSendRequest          = src.bSendRequest;
	bDoTimeout            = src.bDoTimeout;
	szFileName            = src.szFileName;
	szFileSize            = src.szFileSize;
	bResume               = src.bResume;
	bRecvFile             = src.bRecvFile;
	bNoAcks               = src.bNoAcks;
	bIsTdcc               = src.bIsTdcc;
	bOverrideMinimize     = src.bOverrideMinimize;
	bShowMinimized        = src.bShowMinimized;
	bAutoAccept           = src.bAutoAccept;
	bIsIncomingAvatar     = src.bIsIncomingAvatar;
	szLocalFileName       = src.szLocalFileName;
	szLocalFileSize       = src.szLocalFileSize;
#ifdef COMPILE_SSL_SUPPORT
	bIsSSL                = src.bIsSSL;
#endif
	// dcc voice only
	szCodec               = src.szCodec;
	iSampleRate           = src.iSampleRate;
}

bool KviDccDescriptor::isFileUpload()
{
	if(szType.toUpper()=="SEND")return true;
	if(szType.toUpper()=="TSEND")return true;
#ifdef COMPILE_SSL_SUPPORT
	if(szType.toUpper()=="SSEND")return true;
#endif
	return false;
}

bool KviDccDescriptor::isFileDownload()
{
	if(szType.toUpper()=="RECV")return true;
	if(szType.toUpper()=="TRECV")return true;
#ifdef COMPILE_SSL_SUPPORT
	if(szType.toUpper()=="SRECV")return true;
#endif
	return false;
}

bool KviDccDescriptor::isDccChat()
{
	if(szType.toUpper()=="CHAT")return true;
#ifdef COMPILE_SSL_SUPPORT
	if(szType.toUpper()=="SCHAT")return true;
#endif
	return false;
}
