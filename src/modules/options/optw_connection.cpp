//=============================================================================
//
//   File : optw_connection.cpp
//   Creation date : Sat Nov 24 04:25:16 2001 GMT by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2001 Szymon Stefanek (pragma at kvirc dot net)
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

#include "optw_connection.h"

#include "kvi_settings.h"
#include "kvi_locale.h"
#include "kvi_options.h"
#include "kvi_iconmanager.h"


KviConnectionOptionsWidget::KviConnectionOptionsWidget(QWidget * parent)
: KviOptionsWidget(parent,"connection_options_widget")
{

	createLayout(2,1);
	QGroupBox *gbox = addGroupBox(0,0,0,0,1,QGroupBox::Horizontal,__tr2qs_ctx("On Disconnect","options"));


	KviBoolSelector *b1 = addBoolSelector(gbox,__tr2qs_ctx("Keep channels open","options"),KviOption_boolKeepChannelsOpenOnDisconnect,true);
#ifdef COMPILE_INFO_TIPS
	mergeTip(b1,__tr2qs_ctx("<center>This option will cause KVIrc to keep channels open after disconnect.</center>","options"));
#endif

	b1 = addBoolSelector(gbox,__tr2qs_ctx("Keep queries open","options"),KviOption_boolKeepQueriesOpenOnDisconnect,true);
#ifdef COMPILE_INFO_TIPS
	mergeTip(b1,__tr2qs_ctx("<center>This option will cause KVIrc to keep queries open after disconnect.</center>","options"));
#endif

	 gbox = addGroupBox(0,1,0,1,1,QGroupBox::Horizontal,__tr2qs_ctx("On Unexpected Disconnect","options"));


	b1 = addBoolSelector(gbox,__tr2qs_ctx("Keep channels open","options"),KviOption_boolKeepChannelsOpenOnUnexpectedDisconnect,true);
#ifdef COMPILE_INFO_TIPS
	mergeTip(b1,__tr2qs_ctx("<center>This option will cause KVIrc to keep channels open after an unexpected disconnect.</center>","options"));
#endif

	b1 = addBoolSelector(gbox,__tr2qs_ctx("Keep queries open","options"),KviOption_boolKeepQueriesOpenOnUnexpectedDisconnect,true);
#ifdef COMPILE_INFO_TIPS
	mergeTip(b1,__tr2qs_ctx("<center>This option will cause KVIrc to keep queries open after an unexpected disconnect.</center>","options"));
#endif

	b1 = addBoolSelector(gbox,__tr2qs_ctx("Rejoin channels after reconnect","options"),KviOption_boolRejoinChannelsAfterReconnect,KVI_OPTION_BOOL(KviOption_boolAutoReconnectOnUnexpectedDisconnect));
#ifdef COMPILE_INFO_TIPS
	mergeTip(b1,__tr2qs_ctx("<center>This option will cause KVIrc to rejoin channels after a successful reconnect attempt.</center>","options"));
#endif
	b1 = addBoolSelector(gbox,__tr2qs_ctx("Reopen queries after reconnect","options"),KviOption_boolReopenQueriesAfterReconnect,KVI_OPTION_BOOL(KviOption_boolAutoReconnectOnUnexpectedDisconnect));
#ifdef COMPILE_INFO_TIPS
	mergeTip(b1,__tr2qs_ctx("<center>This option will cause KVIrc to reopen query windows after a successful reconnect attempt.</center>","options"));
#endif

//	gbox = addGroupBox(0,1,0,1,1,QGroupBox::Horizontal,__tr2qs_ctx("On Unexpected Disconnect","options"));
	KviBoolSelector * b = addBoolSelector(gbox,__tr2qs_ctx("Automatically reconnect","options"),KviOption_boolAutoReconnectOnUnexpectedDisconnect);
#ifdef COMPILE_INFO_TIPS
	mergeTip(b,__tr2qs_ctx("<center>This option will enable auto-reconnecting after an unexpected disconnect. " \
						"An unexpected disconnect is the <b>termination</b> of a <b>fully connected IRC session</b> " \
						"that was <b>not requested by the user</b> by the means of the QUIT message." \
						"<p><b>Warning:</b> If you use /RAW to send a QUIT message to the server, " \
						"this option will not behave correctly, since does not detect the outgoing " \
						"QUIT message and will attempt to reconnect after the server has closed the connection. " \
						"For this reason, always use the /QUIT command to close your connections. " \
						"This option may also behave incorrectly with bouncers that support " \
						"detaching, in this case a solution could be to prepare an alias that sends the " \
						"bouncer \"detach\" command immediately before the \"quit\" command.<br>" \
						"<tt>alias(bncdetach){ raw bouncer detach; quit; }</tt></p></center>","options"));
#endif
	KviUIntSelector * u = addUIntSelector(gbox,__tr2qs_ctx("Maximum attempts (0: unlimited):","options"),
			KviOption_uintMaxAutoReconnectAttempts,0,100,5,
			KVI_OPTION_BOOL(KviOption_boolAutoReconnectOnUnexpectedDisconnect));
	connect(b,SIGNAL(toggled(bool)),u,SLOT(setEnabled(bool)));

	u = addUIntSelector(gbox,__tr2qs_ctx("Delay between attempts:","options"),
			KviOption_uintAutoReconnectDelay,0,86400,5,
			KVI_OPTION_BOOL(KviOption_boolAutoReconnectOnUnexpectedDisconnect));
	u->setSuffix(__tr2qs_ctx(" sec","options"));
	connect(b,SIGNAL(toggled(bool)),u,SLOT(setEnabled(bool)));
#ifdef COMPILE_INFO_TIPS
	mergeTip(u,__tr2qs_ctx("<center>Minimum value: <b>0 sec</b><br>Maximum value: <b>86400 sec</b></center>","options"));
#endif




	addRowSpacer(0,2,4,2);
}


KviConnectionOptionsWidget::~KviConnectionOptionsWidget()
{
}




KviSSLOptionsWidget::KviSSLOptionsWidget(QWidget * parent)
: KviOptionsWidget(parent,"ssl_options_widget")
{
#ifdef COMPILE_SSL_SUPPORT
	createLayout(3,1);

	QGroupBox * gbox = addGroupBox(0,0,0,0,1,QGroupBox::Horizontal,__tr2qs_ctx("Certificate","options"));

	KviBoolSelector * b = addBoolSelector(gbox,__tr2qs_ctx("Use SSL certificate (PEM format only)","options"),
		&(KVI_OPTION_BOOL(KviOption_boolUseSSLCertificate)),true);
	KviFileSelector * f = addFileSelector(gbox,__tr2qs_ctx("Certificate location:","options"),
		&(KVI_OPTION_STRING(KviOption_stringSSLCertificatePath)),KVI_OPTION_BOOL(KviOption_boolUseSSLCertificate));
	connect(b,SIGNAL(toggled(bool)),f,SLOT(setEnabled(bool)));
	KviPasswordSelector * p = new KviPasswordSelector(gbox,__tr2qs_ctx("Certificate password:","options"),
		&(KVI_OPTION_STRING(KviOption_stringSSLCertificatePass)),KVI_OPTION_BOOL(KviOption_boolUseSSLCertificate));
	connect(b,SIGNAL(toggled(bool)),p,SLOT(setEnabled(bool)));

	gbox = addGroupBox(0,1,0,1,1,QGroupBox::Horizontal,__tr2qs_ctx("Private Key","options"));
	b = addBoolSelector(gbox,__tr2qs_ctx("Use SSL private key","options"),
		&(KVI_OPTION_BOOL(KviOption_boolUseSSLPrivateKey)),true);
	f = addFileSelector(gbox,__tr2qs_ctx("Private key location:","options"),
		&(KVI_OPTION_STRING(KviOption_stringSSLPrivateKeyPath)),KVI_OPTION_BOOL(KviOption_boolUseSSLPrivateKey));
	connect(b,SIGNAL(toggled(bool)),f,SLOT(setEnabled(bool)));
	p = addPasswordSelector(gbox,__tr2qs_ctx("Private key password:","options"),
		&(KVI_OPTION_STRING(KviOption_stringSSLPrivateKeyPass)),KVI_OPTION_BOOL(KviOption_boolUseSSLPrivateKey));
	connect(b,SIGNAL(toggled(bool)),p,SLOT(setEnabled(bool)));
	addRowSpacer(0,2,0,2);
#else
	createLayout(1,1);
	addLabel(0,0,0,0,__tr2qs_ctx("This executable has no SSL support.","options"));
#endif
}

KviSSLOptionsWidget::~KviSSLOptionsWidget()
{
}






KviTransportOptionsWidget::KviTransportOptionsWidget(QWidget * parent)
: KviOptionsWidget(parent,"transport_options_widget")
{
	createLayout(5,1);
	KviUIntSelector * u;
	
	QGroupBox * g = addGroupBox(0,0,0,0,1,QGroupBox::Horizontal,__tr2qs_ctx("Timeout Values","options"),this);
	u = addUIntSelector(g,__tr2qs_ctx("Connect timeout:","options"),KviOption_uintIrcSocketTimeout,5,6000,60);
	u->setSuffix(__tr2qs_ctx(" sec","options"));
	u = addUIntSelector(g,__tr2qs_ctx("Outgoing data queue flush timeout:","options"),KviOption_uintSocketQueueFlushTimeout,100,2000,500);
	u->setSuffix(__tr2qs_ctx(" msec","options"));
	KviBoolSelector * b = addBoolSelector(0,1,0,1,__tr2qs_ctx("Limit outgoing traffic","options"),KviOption_boolLimitOutgoingTraffic);
	u = addUIntSelector(0,2,0,2,__tr2qs_ctx("Limit to 1 message every:","options"),
			KviOption_uintOutgoingTrafficLimitUSeconds,10000,2000000,10000000,KVI_OPTION_BOOL(KviOption_boolLimitOutgoingTraffic));
	u->setSuffix(__tr2qs_ctx(" usec","options"));
#ifdef COMPILE_INFO_TIPS
	mergeTip(u,__tr2qs_ctx("<center>Minimum value: <b>10000 usec</b><br>Maximum value: <b>10000000 usec</b></center>","options"));
#endif
	connect(b,SIGNAL(toggled(bool)),u,SLOT(setEnabled(bool)));

	g = addGroupBox(0,3,0,3,2,QGroupBox::Horizontal,__tr2qs_ctx("Network Interfaces","options"));
	
	b = addBoolSelector(g,__tr2qs_ctx("Bind IPv4 connections to:","options"),KviOption_boolBindIrcIpV4ConnectionsToSpecifiedAddress);
	KviStringSelector * s = addStringSelector(g,"",KviOption_stringIpV4ConnectionBindAddress,KVI_OPTION_BOOL(KviOption_boolBindIrcIpV4ConnectionsToSpecifiedAddress));
	connect(b,SIGNAL(toggled(bool)),s,SLOT(setEnabled(bool)));
#ifdef COMPILE_IPV6_SUPPORT
	b = addBoolSelector(g,__tr2qs_ctx("Bind IPv6 connections to:","options"),KviOption_boolBindIrcIpV6ConnectionsToSpecifiedAddress);
	s = addStringSelector(g,"",KviOption_stringIpV6ConnectionBindAddress,KVI_OPTION_BOOL(KviOption_boolBindIrcIpV6ConnectionsToSpecifiedAddress));
	connect(b,SIGNAL(toggled(bool)),s,SLOT(setEnabled(bool)));
#endif //!COMPILE_IPV6_SUPPORT

	addRowSpacer(0,4,0,4);
}

KviTransportOptionsWidget::~KviTransportOptionsWidget()
{
}









KviIdentOptionsWidget::KviIdentOptionsWidget(QWidget * parent)
: KviOptionsWidget(parent,"ident_options_widget")
{
	createLayout(5,1);

	m_pEnableIdent = addBoolSelector(0,0,0,0,__tr2qs_ctx("Enable ident service (bad practice on UNIX!)","options"),KviOption_boolUseIdentService);
	connect(m_pEnableIdent,SIGNAL(toggled(bool)),this,SLOT(enableIpv4InIpv6(bool)));

	QGroupBox * gbox = addGroupBox(0,1,0,1,1,QGroupBox::Horizontal,__tr2qs_ctx("Configuration","options"),KVI_OPTION_BOOL(KviOption_boolUseIdentService));
	
	KviBoolSelector *b = addBoolSelector(gbox,__tr2qs_ctx("Enable ident service only while connecting to server","options"),KviOption_boolUseIdentServiceOnlyOnConnect);
	connect(m_pEnableIdent,SIGNAL(toggled(bool)),b,SLOT(setEnabled(bool)));

	KviStringSelector * s = addStringSelector(gbox,__tr2qs_ctx("Ident username:","options"),
		KviOption_stringIdentdUser,KVI_OPTION_BOOL(KviOption_boolUseIdentService));
	connect(m_pEnableIdent,SIGNAL(toggled(bool)),s,SLOT(setEnabled(bool)));
	
	KviUIntSelector * u = addUIntSelector(gbox,__tr2qs_ctx("Service port:","options"),
		KviOption_uintIdentdPort,0,65535,113,KVI_OPTION_BOOL(KviOption_boolUseIdentService));
	connect(m_pEnableIdent,SIGNAL(toggled(bool)),u,SLOT(setEnabled(bool)));
	connect(m_pEnableIdent,SIGNAL(toggled(bool)),gbox,SLOT(setEnabled(bool)));

	gbox = addGroupBox(0,2,0,2,1,QGroupBox::Horizontal,__tr2qs_ctx("IPv6 Settings","options"),KVI_OPTION_BOOL(KviOption_boolUseIdentService));
	m_pEnableIpv6 = addBoolSelector(gbox,__tr2qs_ctx("Enable service for IPv6","options"),
		KviOption_boolIdentdEnableIpV6,
		KVI_OPTION_BOOL(KviOption_boolUseIdentService));
#ifdef COMPILE_IPV6_SUPPORT
	connect(m_pEnableIdent,SIGNAL(toggled(bool)),m_pEnableIpv6,SLOT(setEnabled(bool)));
	connect(m_pEnableIpv6,SIGNAL(toggled(bool)),this,SLOT(enableIpv4InIpv6(bool)));
#else
	m_pEnableIpv6->setEnabled(false);
#endif
	m_pIpv4InIpv6 = addBoolSelector(gbox,__tr2qs_ctx("IP stack treats IPv4 as part of IPv6 namespace","options"),
		KviOption_boolIdentdIpV6ContainsIpV4,
		KVI_OPTION_BOOL(KviOption_boolUseIdentService) && KVI_OPTION_BOOL(KviOption_boolIdentdEnableIpV6));
	connect(m_pEnableIdent,SIGNAL(toggled(bool)),gbox,SLOT(setEnabled(bool)));

	addLabel(0,3,0,3,
#ifdef COMPILE_ON_WINDOWS
			__tr2qs_ctx("<p><b>Warning:</b><br>" \
			"This is a <b>non RFC 1413 compliant</b> ident daemon that implements " \
			"only a limited subset of the Identification Protocol specifications. If it is possible, install a " \
			"real ident daemon.</p>","options")
#else
			__tr2qs_ctx("<p><b>Warning:</b><br>" \
			"This is a <b>non RFC 1413 compliant</b> ident daemon that implements " \
			"only a limited subset of the Identification Protocol specifications.<br>" \
			"On UNIX, you may also need root privileges to bind to the auth port (113).<br>" \
			"It is <b>highly recommended</b> that a <b>real</b> system-wide ident daemon be used instead, "\
			"or none at all if ident is not required.</p>","options")
#endif
	);

	addRowSpacer(0,4,0,4);
}

KviIdentOptionsWidget::~KviIdentOptionsWidget()
{
}

void KviIdentOptionsWidget::enableIpv4InIpv6(bool)
{
#ifdef COMPILE_IPV6_SUPPORT
	m_pIpv4InIpv6->setEnabled(m_pEnableIdent->isChecked() && m_pEnableIpv6->isChecked());
#endif
}







KviConnectionAdvancedOptionsWidget::KviConnectionAdvancedOptionsWidget(QWidget * parent)
: KviOptionsWidget(parent,"connection_advanced_options_widget")
{

}

KviConnectionAdvancedOptionsWidget::~KviConnectionAdvancedOptionsWidget()
{

}

#include "m_optw_connection.moc"
