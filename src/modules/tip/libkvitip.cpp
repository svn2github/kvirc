//
//   File : libkvitip.cpp
//   Creation date : Thu May 10 2001 13:50:11 CEST by Szymon Stefanek
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

#include "libkvitip.h"
#include "kvi_module.h"
#include "kvi_styled_controls.h"

#include "kvi_locale.h"
#include "kvi_app.h"

#include "kvi_iconmanager.h"
#include "kvi_options.h"
#include "kvi_fileutils.h"

#include <qpushbutton.h>
#include <qsimplerichtext.h>
#include <qfont.h>
#include <qtextcodec.h>
#include <qpainter.h>

KviTipWindow * g_pTipWindow = 0;

#define KVI_TIP_WINDOW_HEIGHT 200
#define KVI_TIP_WINDOW_WIDTH 500
#define KVI_TIP_WINDOW_BUTTON_WIDTH 80
#define KVI_TIP_WINDOW_BUTTON_HEIGHT 30
#define KVI_TIP_WINDOW_BORDER 5
#define KVI_TIP_WINDOW_DOUBLE_BORDER 10
#define KVI_TIP_WINDOW_SPACING 2


KviTipFrame::KviTipFrame(QWidget * par)
: QFrame(par)
{
	KviStr buffer;
	g_pApp->findImage(buffer,"kvi_tip.png");
	m_pTipPixmap = new QPixmap(buffer.ptr());
	setBackgroundMode(QWidget::NoBackground);
	setFrameStyle(QFrame::Sunken | QFrame::WinPanel);
}

KviTipFrame::~KviTipFrame()
{
	delete m_pTipPixmap;
}

void KviTipFrame::setText(const QString &text)
{
	m_szText = "<center><font color=\"#FFFFFF\">";
	m_szText += text;
	m_szText += "</font></center>";
	update();
}

void KviTipFrame::drawContents(QPainter *p)
{
	p->fillRect(contentsRect(),QColor(0,0,0));
	p->drawPixmap(5,(height() - m_pTipPixmap->height()) / 2,*m_pTipPixmap);

	QFont f = QFont();
	f.setStyleHint(QFont::SansSerif);
	f.setPointSize(12);
	QSimpleRichText doc(m_szText,f);
	doc.setWidth(width() - 80);

	QRegion reg(0,0,1000,20000);

#if QT_VERSION >= 300
	doc.draw(p,70,10,reg,colorGroup());
#else
	doc.draw(p,70,10,reg,palette());
#endif
}

KviTipWindow::KviTipWindow()
: QWidget(0,"kvirc_tip_window" /*,WStyle_Customize | WStyle_Title | WStyle_DialogBorder | WStyle_StaysOnTop*/ )
{
	m_pConfig = 0;


	m_pTipFrame = new KviTipFrame(this);
	m_pTipFrame->setGeometry(
		KVI_TIP_WINDOW_BORDER,
		KVI_TIP_WINDOW_BORDER,
		KVI_TIP_WINDOW_WIDTH - KVI_TIP_WINDOW_DOUBLE_BORDER,
		KVI_TIP_WINDOW_HEIGHT - (KVI_TIP_WINDOW_DOUBLE_BORDER + KVI_TIP_WINDOW_BUTTON_HEIGHT + KVI_TIP_WINDOW_SPACING));

	QPushButton * pb = new QPushButton(">>",this);
	pb->setGeometry(
		KVI_TIP_WINDOW_WIDTH - ((KVI_TIP_WINDOW_BUTTON_WIDTH * 2)+ KVI_TIP_WINDOW_BORDER + KVI_TIP_WINDOW_SPACING),
		KVI_TIP_WINDOW_HEIGHT - (KVI_TIP_WINDOW_BUTTON_HEIGHT + KVI_TIP_WINDOW_BORDER),
		KVI_TIP_WINDOW_BUTTON_WIDTH,
		KVI_TIP_WINDOW_BUTTON_HEIGHT
		);
	connect(pb,SIGNAL(clicked()),this,SLOT(nextTip()));

	pb = new QPushButton(__tr2qs("Close"),this);
	pb->setGeometry(
		KVI_TIP_WINDOW_WIDTH - (KVI_TIP_WINDOW_BUTTON_WIDTH + KVI_TIP_WINDOW_BORDER),
		KVI_TIP_WINDOW_HEIGHT - (KVI_TIP_WINDOW_BUTTON_HEIGHT + KVI_TIP_WINDOW_BORDER),
		KVI_TIP_WINDOW_BUTTON_WIDTH,
		KVI_TIP_WINDOW_BUTTON_HEIGHT
		);
	connect(pb,SIGNAL(clicked()),this,SLOT(close()));
	pb->setDefault(true);

	m_pShowAtStartupCheck = new KviStyledCheckBox(__tr2qs("Show at startup"),this);
	m_pShowAtStartupCheck->setChecked(KVI_OPTION_BOOL(KviOption_boolShowTipAtStartup));
	m_pShowAtStartupCheck->setGeometry(
		KVI_TIP_WINDOW_BORDER,
		KVI_TIP_WINDOW_HEIGHT - (KVI_TIP_WINDOW_BUTTON_HEIGHT + KVI_TIP_WINDOW_BORDER),
		KVI_TIP_WINDOW_WIDTH - ((KVI_TIP_WINDOW_BORDER + KVI_TIP_WINDOW_BUTTON_WIDTH + KVI_TIP_WINDOW_SPACING) * 2),
		KVI_TIP_WINDOW_BUTTON_HEIGHT
		);

	setFixedSize(KVI_TIP_WINDOW_WIDTH,KVI_TIP_WINDOW_HEIGHT);

	setIcon(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_IDEA)));

	setCaption(__tr2qs("Did you know..."));

	pb->setFocus();

}

KviTipWindow::~KviTipWindow()
{
	KVI_OPTION_BOOL(KviOption_boolShowTipAtStartup) = m_pShowAtStartupCheck->isChecked();
	if(m_pConfig)closeConfig();
}

bool KviTipWindow::openConfig(const char * filename,bool bEnsureExists)
{
	if(m_pConfig)closeConfig();

	m_szConfigFileName = filename;
//	m_szConfigFileName.cutToLast('/');

	KviStr buffer;
	g_pApp->getReadOnlyConfigPath(buffer,m_szConfigFileName.ptr(),KviApp::ConfigPlugins,true);

	if(bEnsureExists)
	{
		if(!KviFileUtils::fileExists(buffer.ptr()))return false;
	}

	m_pConfig = new KviConfig(buffer.ptr(),KviConfig::Read);
	
	return true;
}

void KviTipWindow::closeConfig()
{
	KviStr buffer;
	g_pApp->getLocalKvircDirectory(buffer,KviApp::ConfigPlugins,m_szConfigFileName.ptr());
	m_pConfig->setSavePath(buffer.ptr());
	delete m_pConfig;
    m_pConfig = 0;
}

void KviTipWindow::nextTip()
{
	if(!m_pConfig)
	{
		KviStr szLocale = KviLocale::localeName();
		KviStr szFile;
		szFile.sprintf("libkvitip_%s.kvc",szLocale.ptr());
		if(!openConfig(szFile.ptr(),true))
		{
			szLocale.cutFromFirst('.');
			szLocale.cutFromFirst('_');
			szLocale.cutFromFirst('@');
			szFile.sprintf("libkvitip_%s.kvc",szLocale.ptr());
			if(!openConfig(szFile.ptr(),true))
			{
				openConfig("libkvitip.kvc",false);
			}
		}
	}

	unsigned int uNumTips = m_pConfig->readUIntEntry("uNumTips",0);
	unsigned int uNextTip = m_pConfig->readUIntEntry("uNextTip",0);


	KviStr tmp(KviStr::Format,"%u",uNextTip);
	QString szTip = m_pConfig->readEntry(tmp.ptr(),__tr2qs("<b>Can't find any tip... :(</b>"));

	//debug("REDECODED=%s",szTip.utf8().data());

	uNextTip++;
	if(uNextTip >= uNumTips)uNextTip = 0;
	m_pConfig->writeEntry("uNextTip",uNextTip);

	m_pTipFrame->setText(szTip);
}

void KviTipWindow::showEvent(QShowEvent *e)
{
	resize(KVI_TIP_WINDOW_WIDTH,KVI_TIP_WINDOW_HEIGHT);
	move((g_pApp->desktop()->width() - KVI_TIP_WINDOW_WIDTH) / 2,
		(g_pApp->desktop()->height() - KVI_TIP_WINDOW_HEIGHT) / 2);
	QWidget::showEvent(e);
}

void KviTipWindow::closeEvent(QCloseEvent *e)
{
	e->ignore();
	delete this;
	g_pTipWindow = 0;
}

/*
	@doc: tip.open
	@type:
		command
	@title:
		tip.open
	@short:
		Opens the "did you know..." tip window
	@syntax:
		tip.open [tip_file_name:string]
	@description:
		Opens the "did you know..." tip window.<br>
		If <tip_file_name> is specified , that tip is used instead of
		the default tips provided with kvirc.<br>
		<tip_file_name> must be a file name with no path and must refer to a
		standard KVIrc configuration file found in the global or local
		KVIrc plugin configuration directory ($KVIrcDir/config/modules).<br>
		Once the window has been opened, the next tip avaiable in the config file is shown.<br>
		This command works even if the tip window is already opened.<br>
*/


static bool tip_kvs_cmd_open(KviKvsModuleCommandCall * c)
{ 
	QString szTipfilename;
	KVSM_PARAMETERS_BEGIN(c)
		KVSM_PARAMETER("filename",KVS_PT_STRING,KVS_PF_OPTIONAL,szTipfilename)
	KVSM_PARAMETERS_END(c)
	if(!g_pTipWindow)g_pTipWindow = new KviTipWindow();
	if (!szTipfilename.isEmpty()) g_pTipWindow->openConfig(szTipfilename);
	g_pTipWindow->nextTip();
	g_pTipWindow->show();
	return true;
}

static bool tip_module_init(KviModule *m)
{
	KVSM_REGISTER_SIMPLE_COMMAND(m,"open",tip_kvs_cmd_open);
	return true;
}

static bool tip_module_cleanup(KviModule *m)
{
	if(g_pTipWindow)g_pTipWindow->close();
	return true;
}

static bool tip_module_can_unload(KviModule *m)
{
	return (g_pTipWindow == 0);
}

KVIRC_MODULE(
	"Tip",                                              // module name
	"1.0.0",                                                // module version
	"Copyright (C) 2000 Szymon Stefanek (pragma at kvirc dot net)", // author & (C)
	"\"Did you know...\" tip",
	tip_module_init,
	tip_module_can_unload,
	0,
	tip_module_cleanup
)

#include "libkvitip.moc"
