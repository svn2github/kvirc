//
//   File : kvi_proxydb.cpp
//   Creation date : Sat Jul 22 2000 18:23:23 by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 1999-2000 Szymon Stefanek (pragma at kvirc dot net)
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

#define __KVILIB__


#include "kvi_proxydb.h"
#include "kvi_config.h"


KviProxy::KviProxy()
{
	m_szHostname = "proxy.example.net";
	m_uPort      = 1080;
	m_protocol   = Socks4;
	m_bIsIpV6    = false;
}

KviProxy::KviProxy(const KviProxy &prx)
{
	m_szHostname = prx.m_szHostname;
	m_szIp       = prx.m_szIp;
	m_szUser     = prx.m_szUser;
	m_szPass     = prx.m_szPass;
	m_uPort      = prx.m_uPort;
	m_protocol   = prx.m_protocol;
	m_bIsIpV6    = prx.m_bIsIpV6;
}

KviProxy::~KviProxy()
{
}

static const char * proxy_protocols_table[3]= { "SOCKSv4", "SOCKSv5", "HTTP" };

const char * KviProxy::protocolName() const
{
	switch(m_protocol)
	{
		case Socks5: return proxy_protocols_table[1]; break;
		case Http:   return proxy_protocols_table[2]; break;
		default:     return proxy_protocols_table[0]; break;
	}

	return proxy_protocols_table[0];
}

void KviProxy::setNamedProtocol(const char * proto)
{
	if(kvi_strEqualCI(proto,"SOCKSv5"))m_protocol = KviProxy::Socks5;
	else if(kvi_strEqualCI(proto,"HTTP"))m_protocol = KviProxy::Http;
	else m_protocol = KviProxy::Socks4;
}

void KviProxy::getSupportedProtocolNames(QStringList & buf)
{
	for(int i=0;i<3;i++)buf.append(QString(proxy_protocols_table[i]));
}

void KviProxy::normalizeUserAndPass()
{
	m_szUser.stripWhiteSpace();
	m_szPass.stripWhiteSpace();
}

KviProxyDataBase::KviProxyDataBase()
{
	m_pProxyList = new KviPtrList<KviProxy>;
	m_pProxyList->setAutoDelete(true);
	m_pCurrentProxy = 0;
}

KviProxyDataBase::~KviProxyDataBase()
{
	delete m_pProxyList;
}

void KviProxyDataBase::updateProxyIp(const char * proxy,const char * ip)
{
	for(KviProxy * prx = m_pProxyList->first();prx;prx = m_pProxyList->next())
	{
		if(kvi_strEqualCI(proxy,prx->m_szHostname.ptr()))
		{
			prx->m_szIp = ip;
			return;
		}
	}
}

void KviProxyDataBase::clear()
{
	delete m_pProxyList;
	m_pProxyList = new KviPtrList<KviProxy>;
	m_pProxyList->setAutoDelete(true);
	m_pCurrentProxy = 0;
}

void KviProxyDataBase::load(const char * filename)
{
	clear();
	KviConfig cfg(filename,KviConfig::Read);

	unsigned int nEntries = cfg.readUIntEntry("Entries",0);

	for(unsigned int i=0;i<nEntries;i++)
	{
		KviProxy * p = new KviProxy();
		KviStr tmp(KviStr::Format,"%u_Hostname",i);
		p->m_szHostname = cfg.readEntry(tmp.ptr(),"proxy.example.net");
		tmp.sprintf("%u_Port",i);
		p->m_uPort      = cfg.readUIntEntry(tmp.ptr(),7000);
		tmp.sprintf("%u_Ip",i);
		p->m_szIp      = cfg.readEntry(tmp.ptr(),"");
		tmp.sprintf("%u_User",i);
		p->m_szUser    = cfg.readEntry(tmp.ptr(),"");
		tmp.sprintf("%u_Pass",i);
		p->m_szPass    = cfg.readEntry(tmp.ptr(),"");

		tmp.sprintf("%u_Protocol",i);
		KviStr type    = cfg.readEntry(tmp.ptr(),"SOCKSv4");
		p->setNamedProtocol(type.ptr());

		tmp.sprintf("%u_IsIpV6",i);
		p->m_bIsIpV6   = cfg.readBoolEntry(tmp.ptr(),false);
		tmp.sprintf("%u_Current",i);
		if(cfg.readBoolEntry(tmp.ptr(),false))m_pCurrentProxy = p;
		m_pProxyList->append(p);
	}

	if(!m_pCurrentProxy)m_pCurrentProxy = m_pProxyList->first();
}

void KviProxyDataBase::save(const char * filename)
{
	KviConfig cfg(filename,KviConfig::Write);

	cfg.clear();

	cfg.writeEntry("Entries",m_pProxyList->count());


	int i=0;

	for(KviProxy * p=m_pProxyList->first();p;p=m_pProxyList->next())
	{
		KviStr tmp(KviStr::Format,"%u_Hostname",i);
		cfg.writeEntry(tmp.ptr(),p->m_szHostname.ptr());
		tmp.sprintf("%u_Port",i);
		cfg.writeEntry(tmp.ptr(),p->m_uPort);
		tmp.sprintf("%u_Ip",i);
		cfg.writeEntry(tmp.ptr(),p->m_szIp.ptr());
		tmp.sprintf("%u_User",i);
		cfg.writeEntry(tmp.ptr(),p->m_szUser.ptr());
		tmp.sprintf("%u_Pass",i);
		cfg.writeEntry(tmp.ptr(),p->m_szPass.ptr());

		tmp.sprintf("%u_Protocol",i);
		KviStr type;
		switch(p->m_protocol)
		{
			case KviProxy::Socks5: type = "SOCKSv5"; break;
			case KviProxy::Http:   type = "HTTP";   break;
			default:               type = "SOCKSv4"; break;
		}
		cfg.writeEntry(tmp.ptr(),type.ptr());

		tmp.sprintf("%u_IsIpV6",i);
		cfg.writeEntry(tmp.ptr(),p->m_bIsIpV6);
		tmp.sprintf("%u_Current",i);
		if(m_pCurrentProxy == p)cfg.writeEntry(tmp.ptr(),true);
		i++;
	}
}
