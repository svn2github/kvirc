#ifndef _OPTW_HIGHLIGHTING_CPP_
#define _OPTW_HIGHLIGHTING_CPP_
//=============================================================================
//
//   File : optw_highlighting.cpp
//   Creation date : Sat Nov  3 18:09:35 2001 GMT by Szymon Stefanek
//
//   This file is part of the KVIrc irc client distribution
//   Copyright (C) 2001-2010 Szymon Stefanek (pragma at kvirc dot net)
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

#include "optw_highlighting.h"
#include "KviOptions.h"
#include "KviLocale.h"

#include <QLayout>
#include <QLabel>
#include <QRadioButton>

#include "KviTalToolTip.h"

KviAlertHighlightingOptionsWidget::KviAlertHighlightingOptionsWidget(QWidget * parent)
: KviOptionsWidget(parent)
{
	setObjectName("highlighting_options_widget");
}

KviAlertHighlightingOptionsWidget::~KviAlertHighlightingOptionsWidget()
{
}


KviHighlightingOptionsWidget::KviHighlightingOptionsWidget(QWidget * parent)
: KviOptionsWidget(parent)
{
	setObjectName("highlighting_options_widget");

	createLayout();

	KviBoolSelector * b = addBoolSelector(0,0,0,0,__tr2qs_ctx("Enable word highlighting","options"),KviOption_boolUseWordHighlighting);
	KviStringListSelector * sl = addStringListSelector(0,1,0,1,__tr2qs_ctx("Words to highlight:","options"),KviOption_stringlistHighlightWords,KVI_OPTION_BOOL(KviOption_boolUseWordHighlighting));

	connect(b,SIGNAL(toggled(bool)),sl,SLOT(setEnabled(bool)));

	// This can be used even without Word highlighting
	b = addBoolSelector(0,2,0,2,__tr2qs_ctx("Highlight messages containing my nickname","options"),KviOption_boolAlwaysHighlightNick);
	mergeTip(b,__tr2qs_ctx("<center>If this option is enabled, KVIrc will highlight any user message containing your current nickname</center>","options"));


	KviTalGroupBox * gbox = addGroupBox(0,3,0,3,Qt::Horizontal,__tr2qs_ctx("Highlighting Method","options"));

	QGridLayout * g = new QGridLayout(gbox);
	gbox->setLayout(g);

	m_pHighlightAllOccurencesRadioButton = new QRadioButton(__tr2qs_ctx("Highlight every occurence of a word inside text","options"),gbox);
	g->addWidget(m_pHighlightAllOccurencesRadioButton,0,0,1,2);
	m_pHighlightAllOccurencesRadioButton->setChecked(KVI_OPTION_BOOL(KviOption_boolUseFullWordHighlighting));
	KviTalToolTip::add(m_pHighlightAllOccurencesRadioButton,__tr2qs_ctx("<center>This selection will cause KVIrc to search for the highlighted words or nicknames inside the whole text. This will also highlight parts of words (eg. if your nickname is Mark then Markus will be highlighted too).</center>","options"));

	m_pHighlightWholeWordsOnlyRadioButton = new QRadioButton(__tr2qs_ctx("Highlight only whole words splitting on space and on:","options"),gbox);
	g->addWidget(m_pHighlightWholeWordsOnlyRadioButton,1,0,1,1);
	m_pHighlightWholeWordsOnlyRadioButton->setChecked(!KVI_OPTION_BOOL(KviOption_boolUseFullWordHighlighting));
	KviTalToolTip::add(m_pHighlightWholeWordsOnlyRadioButton,__tr2qs_ctx("<center>This selection will first split the string on the specified boundaries (including space) and then will compare all the parts with the highlighted words.</center>","options"));

	m_pWordSplitterCharactersEdit = new QLineEdit(gbox);
	m_pWordSplitterCharactersEdit->setText(KVI_OPTION_STRING(KviOption_stringWordSplitters));
	m_pWordSplitterCharactersEdit->setEnabled(!KVI_OPTION_BOOL(KviOption_boolUseFullWordHighlighting));
	g->addWidget(m_pWordSplitterCharactersEdit,1,1,1,1);

	connect(m_pHighlightWholeWordsOnlyRadioButton,SIGNAL(toggled(bool)),m_pWordSplitterCharactersEdit,SLOT(setEnabled(bool)));

	gbox = addGroupBox(0,4,0,4,Qt::Horizontal,__tr2qs_ctx("Alert Options","options"));

	b = addBoolSelector(gbox,__tr2qs_ctx("Flash the system taskbar entry on highlighted messages","options"),KviOption_boolFlashWindowOnHighlightedMessages);
	mergeTip(b,__tr2qs_ctx("<center>If this option is enabled, KVIrc will (attempt to) flash the system taskbar entry when a highlighted message is printed and KVIrc is not the active window</center>","options"));
	b = addBoolSelector(gbox,__tr2qs_ctx("Popup the notifier window on highlighted messages","options"),KviOption_boolPopupNotifierOnHighlightedMessages);
	mergeTip(b,__tr2qs_ctx("<center>If this option is enabled, KVIrc will popup a little notifier window in the low right corner of your desktop when a highlighted message is printed and KVIrc is not the active window</center>","options"));

	addRowSpacer(0,5,0,5);
}

KviHighlightingOptionsWidget::~KviHighlightingOptionsWidget()
{
}

void KviHighlightingOptionsWidget::commit()
{
	KVI_OPTION_STRING(KviOption_stringWordSplitters) = m_pWordSplitterCharactersEdit->text().trimmed();
	KVI_OPTION_BOOL(KviOption_boolUseFullWordHighlighting) = m_pHighlightAllOccurencesRadioButton->isChecked();

	KviOptionsWidget::commit();
}



KviAlertOptionsWidget::KviAlertOptionsWidget(QWidget * parent)
: KviOptionsWidget(parent)
{
	setObjectName("highlighting_options_widget");

	createLayout();

	KviTalGroupBox * g = addGroupBox(0,0,0,0,Qt::Horizontal,__tr2qs_ctx("Alert Restrictions","options"));
	KviBoolSelector * b3 = addBoolSelector(g,__tr2qs_ctx("Restrict alert","options"),KviOption_boolHighlightOnlyNormalMsg);
	mergeTip(b3, __tr2qs_ctx("<center>If this option is enabled, KVIrc will alert in the window list only if a normal message is received in a channel.<br>Actions like joins, parts and mode changes will be ignored.<br> This is useful if you are in channels with a high rate of traffic and only want to be alerted for messages that are interesting to you.</center>","options"));

	KviBoolSelector * b4 = addBoolSelector(g,__tr2qs_ctx("Alert for highlighted words","options"),KviOption_boolHighlightOnlyNormalMsgHighlightInChanToo,KVI_OPTION_BOOL(KviOption_boolHighlightOnlyNormalMsg));
	mergeTip(b4, __tr2qs_ctx("<center>If this option is enabled, the window list will also alert for messages which contain a word from the highlighted words list above.</center>","options"));

	KviBoolSelector * b5 = addBoolSelector(g,__tr2qs_ctx("Alert for query messages","options"),KviOption_boolHighlightOnlyNormalMsgQueryToo,KVI_OPTION_BOOL(KviOption_boolHighlightOnlyNormalMsg));
	mergeTip(b5, __tr2qs_ctx("<center>If this option is enabled, the window list will also alert for messages which are shown in queries.</center>","options"));
	connect(b3,SIGNAL(toggled(bool)),b4,SLOT(setEnabled(bool)));
	connect(b3,SIGNAL(toggled(bool)),b5,SLOT(setEnabled(bool)));

	KviBoolSelector * b6 = addBoolSelector(g,__tr2qs_ctx("Use custom alert level","options"),KviOption_boolHighlightOnlyAtCostumHighlightLevel);
	mergeTip(b6, __tr2qs_ctx("<center>If this option is enabled, KVIrc will alert in the window list only if the specified alert level is reached.</center>","options"));

	KviUIntSelector * b7 = addUIntSelector(g,__tr2qs_ctx("Minimum alert level:","options"),KviOption_uintMinHighlightLevel,1,KVI_MSGTYPE_MAXLEVEL,KVI_MSGTYPE_MAXLEVEL,KVI_OPTION_BOOL(KviOption_boolHighlightOnlyAtCostumHighlightLevel));
	b7->setEnabled(KVI_OPTION_BOOL(KviOption_boolHighlightOnlyAtCostumHighlightLevel));
	mergeTip(b7, __tr2qs_ctx("<center>This option sets the minimum alert level for the window list.</center>","options"));
	connect(b6,SIGNAL(toggled(bool)),b7,SLOT(setEnabled(bool)));

	addRowSpacer(0,1,0,1);
}

KviAlertOptionsWidget::~KviAlertOptionsWidget()
{
}


#ifndef COMPILE_USE_STANDALONE_MOC_SOURCES
#include "m_optw_highlighting.moc"
#endif //!COMPILE_USE_STANDALONE_MOC_SOURCES

#endif //_OPTW_HIGHLIGHTING_CPP_
