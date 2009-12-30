//=============================================================================
//
//   File : notifiermessage.cpp
//   Creation date : Mar 02 Nov 2004 02:41:18 by Iacopo Palazzi
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2004-2008 Iacopo <iakko@siena.linux.it>
//   Copyright (C) 2009 Fabio Bas < ctrlaltca at gmail dot com >
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

#include "notifiermessage.h"
#include "notifiersettings.h"

#include "kvi_iconmanager.h"
#include "kvi_mirccntrl.h"
#include "kvi_options.h"
#include "kvi_texticonmanager.h"

#include <QRect>
#include <QResizeEvent>

KviNotifierMessage::KviNotifierMessage(QPixmap * pPixmap, const QString &szText)
{
	bool bShowImages=KVI_OPTION_BOOL(KviOption_boolIrcViewShowImages);
	m_szText=szText;
	m_pPixmap=pPixmap;

	if(bShowImages)
	{
		m_pLabel0 = new QLabel();
		m_pLabel0->setFixedSize(16,16);
		if(m_pPixmap)
			m_pLabel0->setPixmap(*m_pPixmap);
	} else {
		m_pLabel0 = 0;
	}


	m_pLabel1 = new QLabel();
	m_pLabel1->setTextFormat(Qt::RichText);
	m_pLabel1->setText(KviNotifierMessage::convertToHtml(m_szText));
	m_pLabel1->setWordWrap(true);

	m_pHBox = new QHBoxLayout(this);
	m_pHBox->setSpacing(SPACING);
	m_pHBox->setMargin(SPACING);
	
	if(bShowImages)
	{
		m_pHBox->setStretch(1,99);
		m_pHBox->addWidget(m_pLabel0);
	}
	m_pHBox->addWidget(m_pLabel1);
}

KviNotifierMessage::~KviNotifierMessage()
{
	if(m_pLabel0)
		m_pLabel0->deleteLater();
	if(m_pLabel1)
		m_pLabel1->deleteLater();
	if(m_pHBox)
		m_pHBox->deleteLater();
}

//TODO merge this code with kvi_topicw.cpp's one
#define KVI_LABEL_DEF_BACK 100
#define KVI_LABEL_DEF_FORE 101

QString KviNotifierMessage::convertToHtml(const QString &szText)
{
	QString szResult;
	bool bCurBold      = false;
	bool bCurUnderline = false;
	bool bIgnoreIcons = false;
	bool bShowIcons = KVI_OPTION_BOOL(KviOption_boolDrawEmoticons);
	
	unsigned char uCurFore      = KVI_LABEL_DEF_FORE; //default fore
	unsigned char uCurBack      = KVI_LABEL_DEF_BACK; //default back

	unsigned int uIdx = 0;

	while(uIdx < (unsigned int)szText.length())
	{
		unsigned short c = szText[(int)uIdx].unicode();
		unsigned int uStart = uIdx;

		while(
				(c != KVI_TEXT_COLOR) &&
				(c != KVI_TEXT_BOLD) &&
				(c != KVI_TEXT_UNDERLINE) &&
				(c != KVI_TEXT_REVERSE) &&
				(c != KVI_TEXT_RESET) &&
				(c != KVI_TEXT_ICON) &&
				((c != ':') || bIgnoreIcons) &&
				((c != ';') || bIgnoreIcons) &&
				((c != '=') || bIgnoreIcons)
			)
		{
			uIdx++;
			bIgnoreIcons=FALSE;
			if(uIdx >= (unsigned int)szText.length())break;
			else c = szText[(int)uIdx].unicode();
		}
		
		bIgnoreIcons=FALSE;
		int iLen = uIdx - uStart;

		if(iLen > 0)
		{
			bool bOpened = FALSE;

			if(uCurFore != KVI_LABEL_DEF_FORE)
			{
				szResult.append("<span style=\"color:");
				szResult.append(KVI_OPTION_MIRCCOLOR(uCurFore).name());
				bOpened = TRUE;
			}

			if(uCurBack != KVI_LABEL_DEF_BACK)
			{
				if(!bOpened)
				{
					szResult.append("<span style=\"background-color:");
					bOpened = TRUE;
				} else {
					szResult.append(";background-color:");
				}
				szResult.append(KVI_OPTION_MIRCCOLOR(uCurBack).name());
			}

			if(bCurUnderline)
			{
				if(!bOpened)
				{
					szResult.append("<span style=\"text-decoration:underline");
					bOpened = TRUE;
				} else {
					szResult.append(";text-decoration:underline");
				}
			}

			if(bCurBold)
			{
				if(!bOpened)
				{
					szResult.append("<span style=\"font-weight:bold");
					bOpened = TRUE;
				} else {
					szResult.append(";font-weight:bold");
				}
			}

			if(bOpened) szResult.append(";\">");

			szResult.append(szText.mid(uStart,iLen));

			if( bOpened )
				szResult.append("</span>");
		}

		switch(c)
		{
			case KVI_TEXT_BOLD: bCurBold = !bCurBold; ++uIdx; break;
			case KVI_TEXT_UNDERLINE: bCurUnderline = !bCurUnderline; ++uIdx; break;
			case KVI_TEXT_REVERSE:
				{
					char auxBack = uCurBack;
					uCurBack = uCurFore;
					uCurFore = auxBack;
				}
				++uIdx;
			break;
			case KVI_TEXT_RESET:
				uCurFore = KVI_LABEL_DEF_FORE;
				uCurBack = KVI_LABEL_DEF_BACK;
				bCurBold = false;
				bCurUnderline = false;
				++uIdx;
			break;
			case KVI_TEXT_COLOR:
			{
				++uIdx;
				unsigned char fore;
				unsigned char back;
				uIdx = getUnicodeColorBytes(szText,uIdx,&fore,&back);
				if(fore != KVI_NOCHANGE)
				{
					uCurFore = fore;
					if(back != KVI_NOCHANGE)uCurBack = back;
				} else {
					// only a CTRL+K
					uCurBack = KVI_LABEL_DEF_BACK;
					uCurFore = KVI_LABEL_DEF_FORE;
				}
			}
			break;
			case ':':
			case ';':
			case '=':
			{
				//potential emoticon, got eyes
				if(bShowIcons)
				{
					++uIdx;
					QString szLookup;
					szLookup.append(QChar(c));
					unsigned short uIsEmoticon=0;
					unsigned int uIcoStart = uIdx;

					if(uIdx < (unsigned int)szText.length())
					{
						//look up for a nose
						if(szText[(int)uIdx] == '-')
						{
							szLookup.append('-');
							uIdx++;
						}
					}

					if(uIdx < (unsigned int)szText.length())
					{
						//look up for a mouth
						unsigned short m = szText[(int)uIdx].unicode();
						switch(m)
						{
							case ')':
							case '(':
							case '/':
							case 'D':
							case 'P':
							case 'S':
							case 'O':
							case '*':
							case '|':
								szLookup+=QChar(m);
								uIsEmoticon++;
								uIdx++;
								break;
							default:
								break;
						}
					}

					if(uIdx < (unsigned int)szText.length())
					{
						//look up for a space
						if(szText[(int)uIdx]== ' ')
						{
							uIsEmoticon++;
						}
					} else {
						//got a smile at the end of the szText
						uIsEmoticon++;
					}

					if(uIsEmoticon>1)
					{
						KviTextIcon * pIcon  = g_pTextIconManager->lookupTextIcon(szLookup);
						// do we have that emoticon-icon association ?
						if(pIcon)
						{
							szResult.append("<img src=\"");
							szResult.append(g_pIconManager->getSmallIconResourceName(pIcon->id()));
							if(uCurBack != KVI_LABEL_DEF_BACK)
							{
								szResult.append("\" style=\"background-color:");
								szResult.append(KVI_OPTION_MIRCCOLOR(uCurBack).name());
							}
							szResult.append("\">");
						} else {
							bIgnoreIcons=TRUE;
							uIdx = uIcoStart-1;
						}
					} else {
						bIgnoreIcons=TRUE;
						uIdx = uIcoStart-1;
					}
				} else {
					bIgnoreIcons=TRUE;
				}
			}
			break;
			case KVI_TEXT_ICON:
			{
				++uIdx;
				if(bShowIcons)
				{
					unsigned int uIcoStart = uIdx;
					while((uIdx < (unsigned int)szText.length()) && (szText[(int)uIdx].unicode() > 32))uIdx++;

					QString szLookup = szText.mid(uIcoStart,uIdx - uIcoStart);

					KviTextIcon * pIcon = g_pTextIconManager->lookupTextIcon(szLookup);
					if(pIcon)
					{
						szResult.append("<img src=\"");
						szResult.append(g_pIconManager->getSmallIconResourceName(pIcon->id()));
						if(uCurBack != KVI_LABEL_DEF_BACK)
						{
							szResult.append("\" style=\"background-color:");
							szResult.append(KVI_OPTION_MIRCCOLOR(uCurBack).name());
						}
						szResult.append("\">");
					} else {
						uIdx = uIcoStart;
					}
				}
			}
			break;
		}
	}
	//qDebug("%s",szResult.toUtf8().data());
	return szResult;
}

