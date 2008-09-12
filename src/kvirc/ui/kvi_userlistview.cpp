//=============================================================================
//
//   File : kvi_userlistview.cpp
//   Creation date : Tue Aug 1 2000 21:05:22 by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2000-2008 Szymon Stefanek (pragma at kvirc dot net)
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


#define _KVI_USERLISTVIEW_CPP_

#include "kvi_debug.h"
#include "kvi_userlistview.h"
#include "kvi_settings.h"
#include "kvi_locale.h"
#include "kvi_options.h"
#include "kvi_defaults.h"
#include "kvi_iconmanager.h"
#include "kvi_regusersdb.h"
#include "kvi_parameterlist.h"
#include "kvi_window.h"
#include "kvi_console.h"
#include "kvi_app.h"
#include "kvi_useraction.h"
#include "kvi_qstring.h"
#include "kvi_frame.h"
#include "kvi_mdimanager.h"
#include "kvi_kvs_eventtriggers.h"
#include "kvi_toolwindows_container.h"
#include "kvi_doublebuffer.h"
#include "kvi_stringconversion.h"
#include "kvi_ircconnection.h"
#include "kvi_ircconnectionserverinfo.h"

#include <QLabel>
#include <QScrollBar>
#include <QPainter>
#include <QPixmap>
#include <QFontMetrics>
#include <QDateTime>
#include <QEvent>
#include <QPaintEvent>

#ifdef COMPILE_PSEUDO_TRANSPARENCY
	extern QPixmap * g_pShadedChildGlobalDesktopBackground;
#endif


// kvi_app.cpp (loaded and destroyed by KviIconManager)
extern QPixmap * g_pUserChanStatePixmap;

// Yet another really complex widget :)

#define KVI_USERLIST_BORDER_WIDTH 2

// FIXME: #warning "We want to be able to navigate the list with the keyboard!"

KviUserListToolTip::KviUserListToolTip(KviUserListView * pView, KviUserListViewArea * pArea)
: KviTalToolTip(pArea)
{
	m_pListView = pView;
}

KviUserListToolTip::~KviUserListToolTip()
{
}

void KviUserListToolTip::maybeTip(const QPoint & pnt)
{
	m_pListView->maybeTip(this,pnt);
}


KviUserListEntry::KviUserListEntry(KviUserListView * pParent, const QString & szNick, KviIrcUserEntry * pEntry, short int iFlags, bool bJoinTimeUnknown)
{
	m_pListView       = pParent;
	m_szNick          = szNick;
	m_pGlobalData     = pEntry;
	m_iFlags          = iFlags;
	m_lastActionTime  = (kvi_time_t)0;
	m_joinTime        = bJoinTimeUnknown ? (kvi_time_t)0 : kvi_unixTime();
	m_iTemperature    = bJoinTimeUnknown ? 0 : KVI_USERACTION_JOIN;

	m_bSelected       = false;
	m_pConnectedAnimation = 0;

	recalcSize();
	updateAvatarData();
}

KviUserListEntry::~KviUserListEntry()
{
	if(m_pConnectedAnimation)
	{
		m_pConnectedAnimation->disconnect(SIGNAL(frameChanged()),
			m_pListView,SLOT(animatedAvatarUpdated()));
	}
}

void KviUserListEntry::resetAvatarConnection()
{
	m_pConnectedAnimation = 0;
}

void KviUserListEntry::updateAvatarData()
{
	KviAnimatedPixmap * pOldMovie = m_pConnectedAnimation;

	m_pConnectedAnimation = 0;

	KviAvatar * pAv = m_pGlobalData->avatar();

	if(KVI_OPTION_BOOL(KviOption_boolShowAvatarsInUserlist))
	{
		if(pAv)
		{
			if(KVI_OPTION_BOOL(KviOption_boolScaleAvatars) &&
					(
						!KVI_OPTION_BOOL(KviOption_boolDoNotStretchAvatars) ||
						(pAv->size().width() > KVI_OPTION_UINT(KviOption_uintAvatarScaleWidth)) ||
						(pAv->size().height() > KVI_OPTION_UINT(KviOption_uintAvatarScaleHeight))
					)
				)
			{
				m_pConnectedAnimation = pAv->forSize(
					KVI_OPTION_UINT(KviOption_uintAvatarScaleWidth),
					KVI_OPTION_UINT(KviOption_uintAvatarScaleHeight)
				);
			}
			else
			{
				m_pConnectedAnimation = pAv->animatedPixmap();
			}
		}
	}

	if(m_pConnectedAnimation != pOldMovie)
	{
		if(KVI_OPTION_BOOL(KviOption_boolEnableAnimatedAvatars))
		{
			m_pConnectedAnimation->start();
		} else {
			m_pConnectedAnimation->stop();
		}

		m_pListView->connect(
			m_pConnectedAnimation,SIGNAL(frameChanged()),
			m_pListView,SLOT(animatedAvatarUpdated()));

		/*
		The old one is deleted.
		So it is disconnected automagically...
		if(pOldMovie)
		{
			m_pListView->disconnect(
						pOldMovie,SIGNAL(frameChanged()),
						m_pListView,SLOT(animatedAvatarUpdated())
						);
		}*/
	}
}

bool KviUserListEntry::color(QColor & color)
{
	//
	// FIXME: Unused ?
	//
	KviRegisteredUser * pRegisteredUser = m_pListView->m_pKviWindow->connection()->userDataBase()->registeredUser(m_szNick);
	if(pRegisteredUser)
	{
		if(m_pListView->m_pKviWindow->connection()->userDataBase()->haveCustomColor(m_szNick))
		{
			color = *(m_pListView->m_pKviWindow->connection()->userDataBase()->customColor(m_szNick));
			return true;
		}
	}

	if(KVI_OPTION_BOOL(KviOption_boolUseDifferentColorForOwnNick) && m_pListView->m_pKviWindow->connection())
	{
		if(m_szNick == m_pListView->m_pKviWindow->connection()->currentNickName())
		{
			color = KVI_OPTION_COLOR(KviOption_colorUserListViewOwnForeground);
			return true;
		}
	}

	if(m_iFlags == 0)
	{
		color = KVI_OPTION_COLOR(KviOption_colorUserListViewNormalForeground);
		return true;
	} else {
		color = KVI_OPTION_COLOR((m_iFlags & KVI_USERFLAG_CHANOWNER) ? \
			KviOption_colorUserListViewChanOwnerForeground : ((m_iFlags & KVI_USERFLAG_CHANADMIN) ? \
			KviOption_colorUserListViewChanAdminForeground : ((m_iFlags & KVI_USERFLAG_OP) ? \
			KviOption_colorUserListViewOpForeground : ((m_iFlags & KVI_USERFLAG_HALFOP) ? \
			KviOption_colorUserListViewHalfOpForeground : ((m_iFlags & KVI_USERFLAG_VOICE) ? \
			KviOption_colorUserListViewVoiceForeground : KviOption_colorUserListViewUserOpForeground)))));
			return true;
	}
	return true;
}

void KviUserListEntry::recalcSize()
{
	KviAvatar * pAv = m_pGlobalData->avatar();
	m_iHeight = m_pListView->m_iFontHeight;
	if(KVI_OPTION_BOOL(KviOption_boolShowUserChannelIcons) && (m_iHeight < 20))m_iHeight = 20;

	if(KVI_OPTION_BOOL(KviOption_boolShowAvatarsInUserlist))//G&N  2005
	{
		if(pAv)
		{
			if(KVI_OPTION_BOOL(KviOption_boolScaleAvatars) &&
					(
						!KVI_OPTION_BOOL(KviOption_boolDoNotStretchAvatars) ||
						(pAv->size().width() > KVI_OPTION_UINT(KviOption_uintAvatarScaleWidth)) ||
						(pAv->size().height() > KVI_OPTION_UINT(KviOption_uintAvatarScaleHeight))
					)
				)
			{
				//don't worry about resize action
				//it has to be done anyway on painting
				//and result will be cached.
				m_iHeight += pAv->forSize(
						KVI_OPTION_UINT(KviOption_uintAvatarScaleWidth),
						KVI_OPTION_UINT(KviOption_uintAvatarScaleHeight)
					)->size().height();
			}
			else
			{
				m_iHeight += pAv->size().height();
			}
		}
	}
	m_iHeight += 3;
}


KviUserListView::KviUserListView(QWidget * pParent, KviWindowToolPageButton * pButton, KviIrcUserDataBase * pDb, KviWindow * pWnd, int iDictSize, const QString & szTextLabel, const char * pName)
: KviWindowToolWidget(pParent,pButton)
{
	setAutoDelete(0);
	setObjectName(pName);

	m_pKviWindow  = pWnd;
	m_pEntryDict  = new KviPointerHashTable<QString,KviUserListEntry>(iDictSize,false);
	m_pEntryDict->setAutoDelete(true);

	m_pUsersLabel = new QLabel(this);
	m_pUsersLabel->setObjectName("userslabel");
	KviTalToolTip::add(m_pUsersLabel,szTextLabel);

	m_pViewArea        = new KviUserListViewArea(this);
	m_pToolTip         = new KviUserListToolTip(this,m_pViewArea);
	m_pTopItem         = 0;
	m_pHeadItem        = 0;
	m_pTailItem        = 0;
	m_iOpCount         = 0;
	m_iHalfOpCount     = 0;
	m_iVoiceCount      = 0;
	m_iChanAdminCount  = 0;
	m_iChanOwnerCount  = 0;
	m_iUserOpCount     = 0;
	m_pIrcUserDataBase = pDb;
	m_iTotalHeight     = 0;
	m_ibEntries        = 0;
	m_ieEntries        = 0;
	m_iIEntries        = 0;
	m_iSelectedCount   = 0;

	applyOptions();
	registerSelf();
}

KviUserListView::~KviUserListView()
{
	removeAllEntries();
	delete m_pEntryDict;
	delete m_pToolTip;
}

void KviUserListView::emitRightClick()
{
	int iEv = -1;
	switch(m_pKviWindow->type())
	{
		case KVI_WINDOW_TYPE_CHANNEL: iEv = KviEvent_OnChannelNickPopupRequest;  break;
		case KVI_WINDOW_TYPE_QUERY:   iEv = KviEvent_OnQueryNickPopupRequest;    break;
		case KVI_WINDOW_TYPE_CONSOLE: iEv = KviEvent_OnNotifyListPopupRequest;   break;
		default:                      iEv = KviEvent_OnNickLinkPopupRequest;     break; // this should actually never happen
	}

	if(iEv > -1)
	{
		if(KviKvsEventManager::instance()->hasAppHandlers(iEv))
		{
			QString szNicks;
			appendSelectedNicknames(szNicks);
			KviKvsVariantList vList;
			vList.append(szNicks);
			KviKvsEventManager::instance()->trigger(iEv,m_pKviWindow,&vList);
		} else {
			g_pApp->checkSuggestRestoreDefaultScript();
		}
	}
}

void KviUserListView::emitDoubleClick()
{
	int iEv = -1;
	switch(m_pKviWindow->type())
	{
		case KVI_WINDOW_TYPE_CHANNEL: iEv = KviEvent_OnChannelNickDefaultActionRequest;  break;
		case KVI_WINDOW_TYPE_QUERY:   iEv = KviEvent_OnQueryNickDefaultActionRequest;    break;
		case KVI_WINDOW_TYPE_CONSOLE: iEv = KviEvent_OnNotifyListDefaultActionRequest;   break;
		default:                      iEv = KviEvent_OnNickLinkDefaultActionRequest;     break; // this should actually never happen
	}

	if(iEv > -1)
	{
		if(KviKvsEventManager::instance()->hasAppHandlers(iEv))
		{
			QString szNicks;
			appendSelectedNicknames(szNicks);
			KviKvsVariantList vList;
			vList.append(szNicks);
			KviKvsEventManager::instance()->trigger(iEv,m_pKviWindow,&vList);
		} else {
			g_pApp->checkSuggestRestoreDefaultScript();
		}
	}
}

void KviUserListView::updateScrollBarRange()
{
	int iMax = m_iTotalHeight - (m_pViewArea->height() - (KVI_USERLIST_BORDER_WIDTH * 2));
	m_pViewArea->m_pScrollBar->setRange(0,iMax > 0 ? iMax : 0);
}

void KviUserListView::applyOptions()
{
/*	if(!KVI_OPTION_BOOL(KviOption_boolDisableUserListLabel)){
		m_pUsersLabel->hide();
		m_pViewArea->setGeometry(0,0,width(),height());
	}
	if(KVI_OPTION_BOOL(KviOption_boolDisableUserListLabel)){
		m_pUsersLabel->show();
		updateUsersLabel();
	}*/
	setFont(KVI_OPTION_FONT(KviOption_fontUserListView));
	QFontMetrics fm(KVI_OPTION_FONT(KviOption_fontUserListView));
	m_iFontHeight = fm.lineSpacing();
	KviUserListEntry * pEntry = m_pHeadItem;
	m_iTotalHeight = 0;
	while(pEntry)
	{
		if(pEntry->m_pConnectedAnimation)
		{
			if(KVI_OPTION_BOOL(KviOption_boolEnableAnimatedAvatars))
			{
				pEntry->m_pConnectedAnimation->start();
			} else {
				pEntry->m_pConnectedAnimation->stop();
			}
		}
		pEntry->recalcSize();
		m_iTotalHeight += pEntry->m_iHeight;
		pEntry = pEntry->m_pNext;
	}
	updateScrollBarRange();
	m_pUsersLabel->setFont(KVI_OPTION_FONT(KviOption_fontUserListView));
	setMinimumWidth(100);
	resizeEvent(0); // this will call update() too
	repaint();
}

void KviUserListView::updateArea()
{
	bool bEnable = m_pViewArea->updatesEnabled();
	if(!bEnable)
		m_pViewArea->setUpdatesEnabled(true);

	triggerUpdate();

	if(!bEnable)
		m_pViewArea->setUpdatesEnabled(false);
}

void KviUserListView::enableUpdates(bool bEnable)
{
	m_pViewArea->setUpdatesEnabled(bEnable);
	if(bEnable)
		triggerUpdate();
}

void KviUserListView::setMaskEntries(char cType, int iNum)
{
	switch(cType)
	{
		case 'b':
			m_ibEntries = iNum;
		break;
		case 'e':
			m_ieEntries = iNum;
		break;
		case 'I':
			m_iIEntries = iNum;
		break;
	}
	updateUsersLabel();
}

void KviUserListView::animatedAvatarUpdated()
{
	//update();
	if(!m_pTopItem)
		return;

	if(!KVI_OPTION_BOOL(KviOption_boolShowAvatarsInUserlist))
		return;

	QRect rct;
	KviUserListEntry * pEntry = m_pTopItem;
	int iCurTop = KVI_USERLIST_BORDER_WIDTH - m_pViewArea->m_iTopItemOffset;
	int iCurBottom = 0;

	while(pEntry && (iCurTop <= m_pViewArea->height()))
	{
		iCurBottom = iCurTop + pEntry->m_iHeight;
		if(pEntry->m_pConnectedAnimation==sender())
		{
			rct.setX(0);
			rct.setY(iCurTop);
			rct.setWidth(m_pViewArea->width());
			rct.setHeight(pEntry->m_iHeight);

			m_pViewArea->update(rct);
		}
		iCurTop = iCurBottom;
		pEntry = pEntry->m_pNext;
	}
}

void KviUserListView::completeNickBashLike(const QString & szBegin, KviPointerList<QString> * pList, bool bAppendMask)
{
	KviUserListEntry * pEntry = m_pHeadItem;

	while(pEntry)
	{
		if(KviQString::equalCIN(szBegin,pEntry->m_szNick,szBegin.length()))
		{
			if(bAppendMask)
			{
				QString * szTmp = new QString();
				KviQString::sprintf(*szTmp,"%Q!%Q@%Q",&(pEntry->m_szNick),&(pEntry->m_pGlobalData->user()),&(pEntry->m_pGlobalData->host()));
				pList->append(szTmp);
			} else
				pList->append(new QString(pEntry->m_szNick));
		}
		pEntry = pEntry->m_pNext;
	}
}

bool KviUserListView::completeNickStandard(const QString & szBegin, const QString & szSkipAfter, QString & szBuffer, bool bAppendMask)
{
	KviUserListEntry * pEntry = m_pHeadItem;

	if(!szSkipAfter.isEmpty())
	{
		while(pEntry)
		{
			if(KviQString::equalCI(szSkipAfter,pEntry->m_szNick))
			{
				pEntry = pEntry->m_pNext;
				break;
			}
			pEntry = pEntry->m_pNext;
		}
	}

	// FIXME: completion should skip my own nick or place it as last entry in the chain (?)

	//	if(KviConsole * c = m_pKviWindow->console())
	//	{
	//		if(kvi_strEqualCI(entry->m_szNick.ptr(),c->currentNickName())
	//	}

	// Ok...now the real completion
	while(pEntry)
	{
		if(pEntry->m_szNick.length() >= szBegin.length())
		{
			int iResult = KviQString::cmpCIN(szBegin,pEntry->m_szNick,szBegin.length());
			if(iResult == 0)
			{
				// This is ok.
				szBuffer = pEntry->m_szNick;
				if(bAppendMask)
				{
					szBuffer += pEntry->m_pGlobalData->user();
					szBuffer += pEntry->m_pGlobalData->host();
				}
				return true;
			}
			/*
			 * This part of code has been commented out to fix #236
			 * This code takes some absurt assterion about the first character of nicknames
			 * and their order in the userlist.

			else if(iResult < 0)
			{
				// No match...begin is lower than the current entry
				if(pEntry->m_iFlags == 0) return false;
				else {
					int iFlags = pEntry->m_iFlags;
					// skip the current flag
					while(pEntry)
					{
						if(pEntry->m_iFlags != iFlags)
							break;
						pEntry = pEntry->m_pNext;
					}
					continue;
				}
			}
			*/
		}
		pEntry = pEntry->m_pNext;
	}

	return false;
}

void KviUserListView::insertUserEntry(const QString & szNnick, KviUserListEntry * pUserEntry)
{
	// Complex insertion task :)

	m_pEntryDict->insert(szNnick,pUserEntry);
	m_iTotalHeight += pUserEntry->m_iHeight;

	bool bGotTopItem = false;

	int iFlag = 0;
	if(pUserEntry->m_iFlags != 0)
	{
		if(pUserEntry->m_iFlags & KVI_USERFLAG_USEROP)
		{
			iFlag = KVI_USERFLAG_USEROP;
			m_iUserOpCount++;
		}

		if(pUserEntry->m_iFlags & KVI_USERFLAG_VOICE)
		{
			iFlag = KVI_USERFLAG_VOICE;
			m_iVoiceCount++;
		}

		if(pUserEntry->m_iFlags & KVI_USERFLAG_HALFOP)
		{
			iFlag = KVI_USERFLAG_HALFOP;
			m_iHalfOpCount++;
		}

		if(pUserEntry->m_iFlags & KVI_USERFLAG_OP)
		{
			iFlag = KVI_USERFLAG_OP;
			m_iOpCount++;
		}

		if(pUserEntry->m_iFlags & KVI_USERFLAG_CHANADMIN)
		{
			iFlag = KVI_USERFLAG_CHANADMIN;
			m_iChanAdminCount++;
		}

		if(pUserEntry->m_iFlags & KVI_USERFLAG_CHANOWNER)
		{
			iFlag = KVI_USERFLAG_CHANOWNER;
			m_iChanOwnerCount++;
		}
	}

	if(m_pHeadItem)
	{
		KviUserListEntry * pEntry = m_pHeadItem;

		if(!(pUserEntry->m_iFlags & KVI_USERFLAG_CHANOWNER))
		{
			// the new user is not a channel owner...
			// skip the channel owners
			while(pEntry && (pEntry->m_iFlags & KVI_USERFLAG_CHANOWNER))
			{
				if(pEntry == m_pTopItem)
					bGotTopItem = true;
				pEntry = pEntry->m_pNext;
			}

			if(!(pUserEntry->m_iFlags & KVI_USERFLAG_CHANADMIN))
			{
				// the new user is not a channel admin...
				// skip chan admins
				while(pEntry && (pEntry->m_iFlags & KVI_USERFLAG_CHANADMIN))
				{
					if(pEntry == m_pTopItem)
						bGotTopItem = true;
					pEntry = pEntry->m_pNext;
				}

				// is operator ?
				if(!(pUserEntry->m_iFlags & KVI_USERFLAG_OP))
				{
					// the new user is not an op...
					// skip ops
					while(pEntry && (pEntry->m_iFlags & KVI_USERFLAG_OP))
					{
						if(pEntry == m_pTopItem)
							bGotTopItem = true;
						pEntry = pEntry->m_pNext;
					}

					// is half oped ?
					if(!(pUserEntry->m_iFlags & KVI_USERFLAG_HALFOP))
					{
						// nope , skip halfops
						while(pEntry && (pEntry->m_iFlags & KVI_USERFLAG_HALFOP))
						{
							if(pEntry == m_pTopItem)
								bGotTopItem = true;
							pEntry = pEntry->m_pNext;
						}

						// is voiced ?
						if(!(pUserEntry->m_iFlags & KVI_USERFLAG_VOICE))
						{
							// nope , not voiced so skip voiced users
							while(pEntry && (pEntry->m_iFlags & KVI_USERFLAG_VOICE))
							{
								if(pEntry == m_pTopItem)
									bGotTopItem = true;
								pEntry = pEntry->m_pNext;
							}

							// is userop'd?
							if(!(pUserEntry->m_iFlags & KVI_USERFLAG_USEROP))
							{
								// nope , skip userops
								while(pEntry && (pEntry->m_iFlags & KVI_USERFLAG_USEROP))
								{
									if(pEntry == m_pTopItem)
	bGotTopItem = true;
									pEntry = pEntry->m_pNext;
								}
							} // else is userop, ops, halfops, and voiced are skipped
						} // else it is voiced , ops and halfops are skipped
					} // else it is halfop ,  ops are skipped
				} // else it is op , chan admins are skipped
			} // else it is chan admin , chan owners are skipped
		} // else it is chan owner, so nothing to skip: the chan owners are first in the list

		// now strcmp within the current user-flag group...
		while(pEntry && (KviQString::cmpCI(pEntry->m_szNick,pUserEntry->m_szNick,
				KVI_OPTION_BOOL(KviOption_boolPlaceNickWithNonAlphaCharsAtEnd)
				) < 0) &&
				((pEntry->m_iFlags & iFlag) || (iFlag == 0)))
		{
			if(pEntry == m_pTopItem)
				bGotTopItem = true;
			pEntry = pEntry->m_pNext;
		}

		if(pEntry)
		{
			// inserting
			pUserEntry->m_pNext = pEntry;
			pUserEntry->m_pPrev = pEntry->m_pPrev;
			if(pUserEntry->m_pPrev == 0)
				m_pHeadItem = pUserEntry;
			else pUserEntry->m_pPrev->m_pNext = pUserEntry;

			pEntry->m_pPrev = pUserEntry;
			// need to adjust the item offsets now...
			// ok... if we're inserting something after
			// the top item, we move everything down
			// otherwise we only update the scrollbar values
			if(!bGotTopItem)
			{
				// Inserting BEFORE the top item
				if((pUserEntry == m_pHeadItem) && (m_pTopItem == pUserEntry->m_pNext) && (m_pViewArea->m_iTopItemOffset == 0))
				{
					// special case...the top item is the head one
					// and it has zero offset...change the top item too
					m_pTopItem = pUserEntry;
					triggerUpdate();
				} else {
					// invisible insertion
					m_pViewArea->m_bIgnoreScrollBar = true;
					m_pViewArea->m_iLastScrollBarVal += pUserEntry->m_iHeight;
					updateScrollBarRange();
					m_pViewArea->m_pScrollBar->setValue(m_pViewArea->m_iLastScrollBarVal);
					m_pViewArea->m_bIgnoreScrollBar = false;
					updateUsersLabel();
				}
			} else {
				triggerUpdate();
			}
		} else {
			// appending to the end (may be visible)
			m_pTailItem->m_pNext = pUserEntry;
			pUserEntry->m_pNext = 0;
			pUserEntry->m_pPrev = m_pTailItem;
			m_pTailItem = pUserEntry;
			triggerUpdate();
		}
	} else {
		// There were no items (is rather visible)
		m_pHeadItem = pUserEntry;
		m_pTailItem = pUserEntry;
		m_pTopItem = pUserEntry;
		pUserEntry->m_pNext = 0;
		pUserEntry->m_pPrev = 0;
		triggerUpdate();
	}

	if(pUserEntry->m_bSelected)
	{
		m_iSelectedCount++;
		if(m_iSelectedCount == 1)
			g_pFrame->childWindowSelectionStateChange(m_pKviWindow,true);
	}
}

KviUserListEntry * KviUserListView::join(const QString & szNick, const QString & szUser, const QString & szHost, int iFlags)
{
	KviUserListEntry * pEntry = m_pEntryDict->find(szNick);
	if(!pEntry)
	{
		// add an entry to the global dict
		KviIrcUserEntry * pGlobalData = m_pIrcUserDataBase->insertUser(szNick,szUser,szHost);
		// calculate the flags and update the counters
		pEntry = new KviUserListEntry(this,szNick,pGlobalData,iFlags,(szUser == QString::null));
		insertUserEntry(szNick,pEntry);
	} else {
//		if(!host.isEmpty()) - it can be UHNAMES with host or NAMEX(X) w/o it
//		{
			// Ok...the user was already on...
			// Probably this is a NAMES(X) reply , and the user IS_ME
			// (already joined after the JOIN message)
			if(iFlags != pEntry->m_iFlags)
			{
//// FIXME: #warning "Maybe say to the channel that we're oped : and the op is guessed from the names reply"
				if((iFlags & KVI_USERFLAG_CHANOWNER) != (pEntry->m_iFlags & KVI_USERFLAG_CHANOWNER))
					setChanOwner(szNick,iFlags & KVI_USERFLAG_CHANOWNER);

				if((iFlags & KVI_USERFLAG_CHANADMIN) != (pEntry->m_iFlags & KVI_USERFLAG_CHANADMIN))
					setChanAdmin(szNick,iFlags & KVI_USERFLAG_CHANADMIN);

				if((iFlags & KVI_USERFLAG_OP) != (pEntry->m_iFlags & KVI_USERFLAG_OP))
					setOp(szNick,iFlags & KVI_USERFLAG_OP);

				if((iFlags & KVI_USERFLAG_HALFOP) != (pEntry->m_iFlags & KVI_USERFLAG_HALFOP))
					setHalfOp(szNick,iFlags & KVI_USERFLAG_HALFOP);

				if((iFlags & KVI_USERFLAG_VOICE) != (pEntry->m_iFlags & KVI_USERFLAG_VOICE))
					setVoice(szNick,iFlags & KVI_USERFLAG_VOICE);

				if((iFlags & KVI_USERFLAG_USEROP) != (pEntry->m_iFlags & KVI_USERFLAG_USEROP))
					setUserOp(szNick,iFlags & KVI_USERFLAG_USEROP);
			}
//		}
	}
	return pEntry;
}

void KviUserListView::triggerUpdate()
{
	// This stuff is useful on joins only
	if(m_pViewArea->updatesEnabled())
	{
		//m_pViewArea->m_pScrollBar->setRange(0,m_iTotalHeight);
		updateScrollBarRange();
		m_pViewArea->update();
		updateUsersLabel();
	}
}

bool KviUserListView::avatarChanged(const QString & szNick)
{
	KviUserListEntry * pUserEntry = m_pEntryDict->find(szNick);
	if(pUserEntry)
	{
		int iOldHeight = pUserEntry->m_iHeight;
		m_iTotalHeight -= pUserEntry->m_iHeight;
		pUserEntry->recalcSize();
		pUserEntry->updateAvatarData();
		m_iTotalHeight += pUserEntry->m_iHeight;
		// if this was "over" the top item , we must adjust the scrollbar value
		// otherwise scroll everything down
		KviUserListEntry * pEntry = m_pHeadItem;
		bool bGotTopItem = false;
		while(pEntry != pUserEntry)
		{
			if(pEntry == m_pTopItem)
			{
				bGotTopItem = true;
				pEntry = pUserEntry;
			} else pEntry = pEntry->m_pNext;
		}

		if(!bGotTopItem && (m_pTopItem != pUserEntry))
		{
			// we're "over" the top item , so over the
			// upper side of the view...adjust the scroll bar value
			int iHeightDiff = pUserEntry->m_iHeight - iOldHeight;
			m_pViewArea->m_iLastScrollBarVal += iHeightDiff;
			m_pViewArea->m_bIgnoreScrollBar = true;
//			m_pViewArea->m_pScrollBar->setRange(0,m_iTotalHeight);
			updateScrollBarRange();
			m_pViewArea->m_pScrollBar->setValue(m_pViewArea->m_iLastScrollBarVal);
			m_pViewArea->m_bIgnoreScrollBar = false;
		} else {
			// the item may be visible!
			// the scroll bar should take care of the case
			// in that the current value runs out of the allowed
			// range.... it should set the value to a good one
			// and emit the signal
			updateScrollBarRange();
//			m_pViewArea->m_pScrollBar->setRange(0,m_iTotalHeight);
			m_pViewArea->update();
		}
		return true;
	}
	return false;
}

bool KviUserListView::userActionVerifyMask(const QString & szNick, const QString & szUser, const QString & szHost, int iActionTemperature, QString & szOldUser, QString & szOldHost)
{
	KviUserListEntry * pEntry = m_pEntryDict->find(szNick);
	if(pEntry)
	{
		pEntry->m_lastActionTime = kvi_unixTime();
		bool bChanged = false;

		if(!(szHost.isEmpty() || (KviQString::equalCS(szHost,"*"))))
		{
			if(!KviQString::equalCI(pEntry->m_pGlobalData->host(),szHost))
			{
				if(!(pEntry->m_pGlobalData->host().isEmpty() || KviQString::equalCS(pEntry->m_pGlobalData->host(),"*")))
				{
					szOldHost = pEntry->m_pGlobalData->host();
					bChanged = true;
				}
				pEntry->m_pGlobalData->setHost(szHost);
			}
		}

		if(!(szUser.isEmpty() || (KviQString::equalCS(szUser,"*"))))
		{
			if(!KviQString::equalCI(pEntry->m_pGlobalData->user(),szUser))
			{
				if(!(pEntry->m_pGlobalData->user().isEmpty() || KviQString::equalCS(pEntry->m_pGlobalData->user(),"*")))
				{
					szOldUser = pEntry->m_pGlobalData->user();
					bChanged = true;
				}
				pEntry->m_pGlobalData->setUser(szUser);
			}
		}

		pEntry->m_iTemperature += iActionTemperature;
		// Don't allow it to grow too much
		if(pEntry->m_iTemperature > 300)
			pEntry->m_iTemperature = 300;
		else if(pEntry->m_iTemperature < -300)
			pEntry->m_iTemperature = -300;

		if(itemVisible(pEntry))
			triggerUpdate();

		return !bChanged;
	}
	return true; // no such nick so no change
}

void KviUserListView::userAction(const QString & szNick, const QString & szUser, const QString & szHost, int iActionTemperature)
{
	KviUserListEntry * pEntry = m_pEntryDict->find(szNick);
	if(pEntry)
	{
		pEntry->m_lastActionTime = kvi_unixTime();
		if(!(szHost.isEmpty() || (KviQString::equalCS(szHost,"*"))))
			pEntry->m_pGlobalData->setHost(szHost);
		if(!(szUser.isEmpty() || (KviQString::equalCS(szUser,"*"))))
			pEntry->m_pGlobalData->setUser(szUser);
		pEntry->m_iTemperature += iActionTemperature;

		// Don't allow it to grow too much
		if(pEntry->m_iTemperature > 300)
			pEntry->m_iTemperature = 300;
		else if(pEntry->m_iTemperature < -300)
			pEntry->m_iTemperature = -300;

		if(itemVisible(pEntry))
			triggerUpdate();
	}
}

void KviUserListView::userAction(KviIrcMask * pUser, int iActionTemperature)
{
	KviUserListEntry * pEntry = m_pEntryDict->find(pUser->nick());
	if(pEntry)
	{
		pEntry->m_lastActionTime = kvi_unixTime();
		if(pUser->hasUser())
			pEntry->m_pGlobalData->setUser(pUser->user());
		if(pUser->hasHost())
			pEntry->m_pGlobalData->setHost(pUser->host());
		pEntry->m_iTemperature += iActionTemperature;

		// Don't allow it to grow too much
		if(pEntry->m_iTemperature > 300)
			pEntry->m_iTemperature = 300;
		else if(pEntry->m_iTemperature < -300)
			pEntry->m_iTemperature = -300;

		if(itemVisible(pEntry))
			triggerUpdate();
	}
}

void KviUserListView::userAction(const QString & szNick, int iActionTemperature)
{
	KviUserListEntry * pEntry = m_pEntryDict->find(szNick);
	if(pEntry)
	{
		pEntry->m_lastActionTime = kvi_unixTime();
		pEntry->m_iTemperature += iActionTemperature;

		if(pEntry->m_iTemperature > 300)
			pEntry->m_iTemperature = 300;
		else if(pEntry->m_iTemperature < -300)
			pEntry->m_iTemperature = -300;

		if(itemVisible(pEntry))
			triggerUpdate();
	}
}

kvi_time_t KviUserListView::getUserJoinTime(const QString & szNick)
{
	KviUserListEntry * pEntry = m_pEntryDict->find(szNick);
	if(!pEntry)
		return (kvi_time_t)0;

	return pEntry->m_joinTime;
}

kvi_time_t KviUserListView::getUserLastActionTime(const QString & szNick)
{
	KviUserListEntry * pEntry = m_pEntryDict->find(szNick);
	if(!pEntry)
		return (kvi_time_t)0;

	return pEntry->m_lastActionTime;
}

int KviUserListView::getUserModeLevel(const QString & szNick)
{
	KviUserListEntry * pEntry = m_pEntryDict->find(szNick);
	if(!pEntry)
		return 0;

	if(pEntry->m_iFlags & KVI_USERFLAG_MODEMASK)
	{
		if(pEntry->m_iFlags & KVI_USERFLAG_CHANOWNER)
			return 60;
		if(pEntry->m_iFlags & KVI_USERFLAG_CHANADMIN)
			return 50;
		if(pEntry->m_iFlags & KVI_USERFLAG_OP)
			return 40;
		if(pEntry->m_iFlags & KVI_USERFLAG_HALFOP)
			return 30;
		if(pEntry->m_iFlags & KVI_USERFLAG_VOICE)
			return 20;
		if(pEntry->m_iFlags & KVI_USERFLAG_USEROP)
			return 10;
	}
	return 0;
}

char KviUserListView::getUserFlag(KviUserListEntry * pEntry)
{
	if(!pEntry)
		return 0;

	return (char)m_pKviWindow->connection()->serverInfo()->modePrefixChar(pEntry->m_iFlags).unicode();
}

void KviUserListView::prependUserFlag(const QString & szNick, QString & szBuffer)
{
	char uFlag = getUserFlag(szNick);
	if(uFlag)
		szBuffer.prepend(uFlag);
}

int KviUserListView::flags(const QString & szNick)
{
	KviUserListEntry * pEntry = m_pEntryDict->find(szNick);
	return pEntry ? pEntry->m_iFlags : 0;
}

#define SET_FLAG_FUNC(__funcname,__flag) \
	bool KviUserListView::__funcname(const QString & szNick, bool bYes) \
	{ \
		KviUserListEntry * pEntry = m_pEntryDict->find(szNick); \
		if(!pEntry) \
			return false; \
		m_pEntryDict->setAutoDelete(false); \
		partInternal(szNick,false); \
		m_pEntryDict->setAutoDelete(true); \
		if(bYes) \
		{ \
			if(!(pEntry->m_iFlags & __flag)) \
				pEntry->m_iFlags |= __flag; \
		} else { \
			if(pEntry->m_iFlags & __flag) \
				pEntry->m_iFlags &= ~__flag; \
		} \
		updateScrollBarRange(); \
		insertUserEntry(szNick,pEntry); \
		m_pViewArea->update(); \
		return true; \
	}

SET_FLAG_FUNC(setChanOwner,KVI_USERFLAG_CHANOWNER)
SET_FLAG_FUNC(setChanAdmin,KVI_USERFLAG_CHANADMIN)
SET_FLAG_FUNC(setOp,KVI_USERFLAG_OP)
SET_FLAG_FUNC(setHalfOp,KVI_USERFLAG_HALFOP)
SET_FLAG_FUNC(setUserOp,KVI_USERFLAG_USEROP)
SET_FLAG_FUNC(setVoice,KVI_USERFLAG_VOICE)

#define GET_FLAG_FUNC(__funcname,__flag) \
	bool KviUserListView::__funcname(const QString & szNick, bool bAtLeast) \
	{ \
		KviUserListEntry * pEntry = m_pEntryDict->find(szNick); \
		return pEntry ? (bAtLeast ? (pEntry->m_iFlags >= __flag) : (pEntry->m_iFlags & __flag)) : false; \
	}

GET_FLAG_FUNC(isChanOwner,KVI_USERFLAG_CHANOWNER)
GET_FLAG_FUNC(isChanAdmin,KVI_USERFLAG_CHANADMIN)
GET_FLAG_FUNC(isOp,KVI_USERFLAG_OP)
GET_FLAG_FUNC(isVoice,KVI_USERFLAG_VOICE)
GET_FLAG_FUNC(isHalfOp,KVI_USERFLAG_HALFOP)
GET_FLAG_FUNC(isUserOp,KVI_USERFLAG_USEROP)

QString * KviUserListView::firstSelectedNickname()
{
	m_pIterator = m_pHeadItem;
	while(m_pIterator)
	{
		if(m_pIterator->m_bSelected)
		{
			QString * szTmp = &(m_pIterator->m_szNick);
			m_pIterator = m_pIterator->m_pNext;
			return szTmp;
		}
		m_pIterator = m_pIterator->m_pNext;
	}
	return 0;
}

QString * KviUserListView::nextSelectedNickname()
{
	while(m_pIterator)
	{
		if(m_pIterator->m_bSelected)
		{
			QString * szTmp = &(m_pIterator->m_szNick);
			m_pIterator = m_pIterator->m_pNext;
			return szTmp;
		}
		m_pIterator = m_pIterator->m_pNext;
	}
	return 0;
}

void KviUserListView::appendSelectedNicknames(QString & szBuffer)
{
	KviUserListEntry * pEntry = m_pHeadItem;
	bool bFirst = true;
	while(pEntry)
	{
		if(pEntry->m_bSelected)
		{
			if(!bFirst)
				szBuffer.append(',');
			else bFirst = false;
			szBuffer.append(pEntry->m_szNick);
		}
		pEntry = pEntry->m_pNext;
	}
}

void KviUserListView::select(const QString & szNick)
{
	KviPointerHashTableIterator<QString,KviUserListEntry> it(*m_pEntryDict);
	while(it.current())
	{
		((KviUserListEntry *)it.current())->m_bSelected = false;
		++it;
	}

	KviUserListEntry * pEntry = m_pEntryDict->find(szNick);
	if(pEntry)
	{
		pEntry->m_bSelected = true;
		m_iSelectedCount = 1;
	} else {
		m_iSelectedCount = 0;
	}

	g_pFrame->childWindowSelectionStateChange(m_pKviWindow,true);
	m_pViewArea->update();
}

bool KviUserListView::partInternal(const QString & szNick, bool bRemove)
{
	KviUserListEntry * pUserEntry = m_pEntryDict->find(szNick);
	if(pUserEntry)
	{
		// so, first of all..check if this item is over, or below the top item
		KviUserListEntry * pEntry = m_pHeadItem;
		bool bGotTopItem = false;
		while(pEntry != pUserEntry)
		{
			if(pEntry == m_pTopItem)
			{
				bGotTopItem = true;
				pEntry = pUserEntry;
			} else pEntry = pEntry->m_pNext;
		}

		if(bRemove)
			m_pIrcUserDataBase->removeUser(szNick,pUserEntry->m_pGlobalData);

		if(!m_pIrcUserDataBase->find(szNick))
		{
			//completelly removed. avatar is deleted
			pUserEntry->resetAvatarConnection();
		}

		// now just remove it
		if(pUserEntry->m_iFlags & KVI_USERFLAG_OP)
			m_iOpCount--;
		if(pUserEntry->m_iFlags & KVI_USERFLAG_VOICE)
			m_iVoiceCount--;
		if(pUserEntry->m_iFlags & KVI_USERFLAG_HALFOP)
			m_iHalfOpCount--;
		if(pUserEntry->m_iFlags & KVI_USERFLAG_CHANADMIN)
			m_iChanAdminCount--;
		if(pUserEntry->m_iFlags & KVI_USERFLAG_CHANOWNER)
			m_iChanOwnerCount--;
		if(pUserEntry->m_iFlags & KVI_USERFLAG_USEROP)
			m_iUserOpCount--;
		if(pUserEntry->m_bSelected)
		{
			m_iSelectedCount--;
			if(m_iSelectedCount == 0)
				g_pFrame->childWindowSelectionStateChange(m_pKviWindow,false);
		}
		if(pUserEntry->m_pPrev)
			pUserEntry->m_pPrev->m_pNext = pUserEntry->m_pNext;
		if(pUserEntry->m_pNext)
			pUserEntry->m_pNext->m_pPrev = pUserEntry->m_pPrev;
		if(m_pTopItem == pUserEntry)
		{
			bGotTopItem = true; // !!! the previous while() does not handle it!
			m_pTopItem = pUserEntry->m_pNext;
			if(m_pTopItem == 0)
				m_pTopItem = pUserEntry->m_pPrev;
		}
		if(pUserEntry == m_pHeadItem)
			m_pHeadItem = pUserEntry->m_pNext;
		if(pUserEntry == m_pTailItem)
			m_pTailItem = pUserEntry->m_pPrev;
		m_iTotalHeight -= pUserEntry->m_iHeight;

		int iHeight = pUserEntry->m_iHeight;

		m_pEntryDict->remove(szNick);

		if(bGotTopItem)
		{
			// removing after (or exactly) the top item, may be visible
			if(bRemove)
				triggerUpdate();
		} else {
			// removing over (before) the top item...not visible
			m_pViewArea->m_bIgnoreScrollBar = true;
			m_pViewArea->m_iLastScrollBarVal -= iHeight;
			m_pViewArea->m_pScrollBar->setValue(m_pViewArea->m_iLastScrollBarVal);
//			m_pViewArea->m_pScrollBar->setRange(0,m_iTotalHeight);
			updateScrollBarRange();
			m_pViewArea->m_bIgnoreScrollBar = false;
			if(bRemove)
				updateUsersLabel();
		}

		return true;
	}
	return false;
}

bool KviUserListView::nickChange(const QString & szOldNick, const QString & szNewNick)
{
	KviUserListEntry * pEntry = m_pEntryDict->find(szOldNick);
	if(pEntry)
	{
		QString szUser    = pEntry->m_pGlobalData->user();
		QString szHost    = pEntry->m_pGlobalData->host();
		int iFlags        = pEntry->m_iFlags;
		kvi_time_t joint  = pEntry->m_joinTime;
		bool bSelect      = pEntry->m_bSelected;
		KviAvatar * pAv   = pEntry->m_pGlobalData->forgetAvatar();
		KviIrcUserEntry::Gender gender = pEntry->m_pGlobalData->gender();
		bool bBot = pEntry->m_pGlobalData->isBot();
		part(szOldNick);
		__range_invalid(m_pEntryDict->find(szOldNick));

		pEntry = join(szNewNick,szUser,szHost,iFlags);
		pEntry->m_pGlobalData->setGender(gender);
		pEntry->m_pGlobalData->setBot(bBot);
		pEntry->m_joinTime = joint;
		pEntry->m_lastActionTime = kvi_unixTime();
		pEntry->m_bSelected = bSelect;
		pEntry->m_iTemperature += KVI_USERACTION_NICK;

		if(pAv)
		{
			pEntry->m_pGlobalData->setAvatar(pAv);
			avatarChanged(szNewNick);
		}
		return true;
	}
	return false;
}

void KviUserListView::updateUsersLabel()
{
	if(KVI_OPTION_BOOL(KviOption_boolShowUserListStatisticLabel))//G&N  2005
	{
		KviStr tmp;
		tmp.sprintf("<nobr><b>[%u]</b>",m_pEntryDict->count());
		if(m_iChanOwnerCount)
			tmp.append(KviStr::Format," q:%d",m_iChanOwnerCount);
		if(m_iChanAdminCount)
			tmp.append(KviStr::Format," a:%d",m_iChanAdminCount);
		if(m_iOpCount)
			tmp.append(KviStr::Format," o:%d",m_iOpCount);
		if(m_iHalfOpCount)
			tmp.append(KviStr::Format," h:%d",m_iHalfOpCount);
		if(m_iVoiceCount)
			tmp.append(KviStr::Format," v:%d",m_iVoiceCount);
		if(m_iUserOpCount)
			tmp.append(KviStr::Format," u:%d",m_iUserOpCount);
		if(m_ibEntries)
			tmp.append(KviStr::Format," b:%d",m_ibEntries);
		if(m_ieEntries)
			tmp.append(KviStr::Format," e:%d",m_ieEntries);
		if(m_iIEntries)
			tmp.append(KviStr::Format," I:%d",m_iIEntries);
		tmp.append("</nobr>");
		m_pUsersLabel->setText(tmp.ptr());
	}
}

// FIXME: this could be done really better
void KviUserListView::partAllButOne(const QString & szWhoNot)
{
	QStringList list;
	KviPointerHashTableIterator<QString,KviUserListEntry> it(*m_pEntryDict);
	while(it.current())
	{
		if(!KviQString::equalCI(szWhoNot,it.currentKey()))
			list.append(it.currentKey());
		++it;
	}
	for(QStringList::Iterator it2 = list.begin();it2 != list.end();it2++)
	{
		part(*it2);
	}
}

void KviUserListView::removeAllEntries()
{
	KviPointerHashTableIterator<QString,KviUserListEntry> it(*m_pEntryDict);
	while(it.current())
	{
		it.current()->resetAvatarConnection();
		m_pIrcUserDataBase->removeUser(it.currentKey(),
			((KviUserListEntry *)it.current())->m_pGlobalData);
		++it;
	}

	m_pEntryDict->clear();
	m_pHeadItem = 0;
	m_pTopItem = 0;
	m_iVoiceCount = 0;
	m_iHalfOpCount = 0;
	m_iChanAdminCount = 0;
	m_iChanOwnerCount = 0;
	m_iOpCount = 0;
	m_iUserOpCount = 0;

	if(m_iSelectedCount != 0)
	{
		m_iSelectedCount = 0;
		g_pFrame->childWindowSelectionStateChange(m_pKviWindow,false);
	}

	m_pViewArea->m_iTopItemOffset = 0;
	m_pViewArea->m_iLastScrollBarVal = 0;
	m_pViewArea->m_bIgnoreScrollBar = true;
	m_pViewArea->m_pScrollBar->setValue(0);
	m_iTotalHeight = 0;
	updateScrollBarRange();
	m_pViewArea->m_bIgnoreScrollBar = false;  // gfgf
}

void KviUserListView::partAll()
{
	removeAllEntries();
	triggerUpdate();
}

void KviUserListView::resizeEvent(QResizeEvent * e)
{
	int iHeight;
	if(KVI_OPTION_BOOL(KviOption_boolShowUserListStatisticLabel))//G&N  2005
	{
		iHeight = m_pUsersLabel->sizeHint().height();
		if(iHeight < 16)
			iHeight = 16; // at least
		m_pUsersLabel->setGeometry(0,0,width(),iHeight);
	} else {
		iHeight = 0;
	}

	m_pViewArea->setGeometry(0,iHeight,width(),height() - iHeight);

	updateScrollBarRange();
}

bool KviUserListView::itemVisible(KviUserListEntry * e)
{
	KviUserListEntry * pEntry = m_pTopItem;
	int iCurTop = KVI_USERLIST_BORDER_WIDTH - m_pViewArea->m_iTopItemOffset;
	int iHeight = height();
	while(pEntry && (iCurTop < iHeight))
	{
		if(pEntry == e)
			return true;
		iCurTop += pEntry->m_iHeight;
		pEntry = pEntry->m_pNext;
	}
	return false;
}

KviUserListEntry * KviUserListView::itemAt(const QPoint & pnt, QRect * pRect)
{
	if(!m_pTopItem)
		return 0;
	if(pnt.y() < 0)
		return 0;

	int iCurTop = KVI_USERLIST_BORDER_WIDTH - m_pViewArea->m_iTopItemOffset;
	int iCurBottom = 0;
	KviUserListEntry * pEntry = m_pTopItem;
	while(pEntry && (iCurTop <= m_pViewArea->height()))
	{
		iCurBottom = iCurTop + pEntry->m_iHeight;
		if((pnt.y() >= iCurTop) && (pnt.y() < iCurBottom))
		{
			if(pRect)
			{
				pRect->setX(0);
				pRect->setY(iCurTop);
				pRect->setWidth(m_pViewArea->width());
				pRect->setHeight(pEntry->m_iHeight);
			}
			return pEntry;
		}
		iCurTop = iCurBottom;
		pEntry = pEntry->m_pNext;
	}
	return 0;
}

void KviUserListView::userStats(KviUserListViewUserStats * pStats)
{
	pStats->uTotal = m_pEntryDict->count();
	pStats->uHot = 0;
	pStats->uHotOp = 0;
	pStats->uActive = 0;
	pStats->uActiveOp = 0;
	pStats->uChanAdmin = 0;
	pStats->uChanOwner = 0;
	pStats->iAvgTemperature = 0;
	pStats->uOp = 0;
	pStats->uVoiced = 0;
	pStats->uHalfOp = 0;
	pStats->uUserOp = 0;

	KviUserListEntry * pEntry = m_pHeadItem;

	kvi_time_t curTime = kvi_unixTime();

	while(pEntry)
	{
		if(pEntry->m_lastActionTime)
		{
			unsigned int uTimeDiff = (((unsigned int)(curTime - pEntry->m_lastActionTime)) >> 6);
			if(uTimeDiff < 10)
			{
				pStats->uActive++; // the user was alive in the last ~16 mins
				if(pEntry->m_iFlags & (KVI_USERFLAG_OP | KVI_USERFLAG_CHANADMIN | KVI_USERFLAG_CHANOWNER))
				{
					pStats->uActiveOp++;
					if(pEntry->m_iTemperature > 0)
					{
						pStats->uHot++;
						pStats->uHotOp++;
					}
				} else {
					if(pEntry->m_iTemperature > 0)
						pStats->uHot++;
				}
				pStats->iAvgTemperature += pEntry->m_iTemperature;
			}
		}
		if(pEntry->m_iFlags & KVI_USERFLAG_CHANOWNER)
			pStats->uChanOwner++;
		else {
			if(pEntry->m_iFlags & KVI_USERFLAG_CHANADMIN)
				pStats->uChanAdmin++;
			else {
				if(pEntry->m_iFlags & KVI_USERFLAG_OP)
					pStats->uOp++;
				else {
					if(pEntry->m_iFlags & KVI_USERFLAG_HALFOP)
						pStats->uHalfOp++;
					else {
						if(pEntry->m_iFlags & KVI_USERFLAG_VOICE)
							pStats->uVoiced++;
						else {
							if(pEntry->m_iFlags & KVI_USERFLAG_USEROP)
								pStats->uUserOp++;
						}
					}
				}
			}
		}
		pEntry = pEntry->m_pNext;
	}

	if(pStats->uActive > 0)
		pStats->iAvgTemperature /= ((int)pStats->uActive);
}

void KviUserListView::maybeTip(KviUserListToolTip * pTip, const QPoint & pnt)
{
	if(!KVI_OPTION_BOOL(KviOption_boolShowUserListViewToolTips))
		return;
	QRect itRect;
	KviUserListEntry * pEntry = (KviUserListEntry *)itemAt(pnt,&itRect);
	if(pEntry)
	{
		if(m_pKviWindow->console())
		{
			QString szBuffer;
			m_pKviWindow->console()->getUserTipText(pEntry->m_szNick,pEntry->m_pGlobalData,szBuffer);

			szBuffer += "<table width=\"100%\">";

			if(pEntry->m_joinTime != 0)
			{
				QDateTime dt;
				dt.setTime_t(pEntry->m_joinTime);
				szBuffer += "<tr><td bgcolor=\"#F0F0F0\"><nobr><font color=\"#000000\">";
				szBuffer += __tr2qs("Joined on <b>%1</b>").arg(dt.toString());
				szBuffer += "</font></nobr></td></tr>";
			}

			if(pEntry->m_lastActionTime != 0)
			{
				int iSecs = kvi_unixTime() - pEntry->m_lastActionTime;
				int iMins = iSecs / 60;
				iSecs = iSecs % 60;
				int iHours = iMins / 60;
				iMins = iMins % 60;
				szBuffer += "<tr><td bgcolor=\"#F0F0F0\"><nobr><font color=\"#000000\">";
				szBuffer += __tr2qs("Quiet for <b>%1h %2m %3s</b>").arg(iHours).arg(iMins).arg(iSecs);
				szBuffer += "</font></nobr></td></tr>";
			}
			szBuffer += "</table>";

			pTip->doTip(itRect,szBuffer);
		}
	}
}

KviUserListViewArea::KviUserListViewArea(KviUserListView * pPar)
: QWidget(pPar)
{
	m_pListView = pPar;
	setAutoFillBackground(false);

	m_pScrollBar = new QScrollBar(Qt::Vertical,this);
	m_pScrollBar->setObjectName("scrollbar");
	m_pScrollBar->setRange(0,0);
	m_pScrollBar->setValue(0);
	connect(m_pScrollBar,SIGNAL(valueChanged(int)),this,SLOT(scrollBarMoved(int)));
	m_pScrollBar->setPageStep(height());
	m_pScrollBar->setSingleStep(m_pListView->m_iFontHeight);
	m_iLastScrollBarVal = 0;
	m_iTopItemOffset = 0;
	m_bIgnoreScrollBar = false;
	m_pLastEntryUnderMouse = 0;
}

KviUserListViewArea::~KviUserListViewArea()
{
}

void KviUserListViewArea::scrollBarMoved(int iNewVal)
{
	if(m_bIgnoreScrollBar)
		return;
	int iDiff = iNewVal - m_iLastScrollBarVal;
	if(m_pListView->m_pTopItem)
	{
		while(iDiff > 0)
		{
			int iNextH = (m_pListView->m_pTopItem->m_iHeight - m_iTopItemOffset);
			if(iDiff >= iNextH)
			{
				// the diff is greater than the top item visible part
				iDiff -= iNextH;
				if(m_pListView->m_pTopItem->m_pNext)
				{
					// There is a next item (offset to 0)
					m_pListView->m_pTopItem = m_pListView->m_pTopItem->m_pNext;
					m_iTopItemOffset = 0;
				} else {
					// No next item (rather a bug) (offset to the top item size)
					m_iTopItemOffset = m_pListView->m_pTopItem->m_iHeight;
					iDiff = 0;
				}
			} else {
				// just offset the top item
				m_iTopItemOffset += iDiff;
				iDiff = 0;
			}
		}

		while(iDiff < 0)
		{
			if((-iDiff) <= m_iTopItemOffset)
			{
				// just move the top item
				m_iTopItemOffset += iDiff;
				iDiff = 0;
			} else {
				iDiff += m_iTopItemOffset;
				if(m_pListView->m_pTopItem->m_pPrev)
				{
					// There is a prev item (offset to 0)
					m_pListView->m_pTopItem = m_pListView->m_pTopItem->m_pPrev;
					m_iTopItemOffset = m_pListView->m_pTopItem->m_iHeight;
				} else {
					// No prev item (rather a bug) (offset to the top item size)
					m_iTopItemOffset = 0;
					iDiff = 0;
				}
			}
		}
	}
	m_iLastScrollBarVal = iNewVal;
	update();
}

void KviUserListViewArea::paintEvent(QPaintEvent * e)
{
	// update the scroll bar
	// if the mdiManager is in SDI mode
	// and this window is attacched but is not the toplevel one
	// then it is hidden completely behind the other windows
	// and we can avoid to paint it :)
	if(!isVisible())
		return;

	if(g_pFrame->mdiManager()->isInSDIMode() &&
		(m_pListView->window()->mdiParent() != g_pFrame->mdiManager()->topChild()) &&
		(m_pListView->window()->mdiParent()))
	{
		return; // totally hidden behind other windows
	}

	int iWidth = width() - m_pScrollBar->width();

	QRect r = e->rect();
	if(r.right() > iWidth)
		r.setRight(iWidth);

	//debug("PAINT EVENT %d,%d,%d,%d",r.left(),r.top(),r.width(),r.height());

	KviDoubleBuffer db(width(),height());
	QPixmap * pMemBuffer = db.pixmap();

	QPainter p(pMemBuffer);
	SET_ANTI_ALIASING(p);
	p.setFont(KVI_OPTION_FONT(KviOption_fontUserListView));

	QFontMetrics fm(p.fontMetrics());

#ifdef COMPILE_PSEUDO_TRANSPARENCY
	if(g_pShadedChildGlobalDesktopBackground)
	{
		QPoint pnt = mapToGlobal(QPoint(r.left(),r.top()));
		p.drawTiledPixmap(r.left(),r.top(),r.width(),r.height(),*g_pShadedChildGlobalDesktopBackground,pnt.x(),pnt.y());
	} else {
#endif
		QPixmap * pPix = KVI_OPTION_PIXMAP(KviOption_pixmapUserListViewBackground).pixmap();
		p.fillRect(r.left(),r.top(),r.width(),r.height(),KVI_OPTION_COLOR(KviOption_colorUserListViewBackground));
		if(pPix)
			KviPixmapUtils::drawPixmapWithPainter(&p,pPix,KVI_OPTION_UINT(KviOption_uintUserListPixmapAlign),r,width(),height());
#ifdef COMPILE_PSEUDO_TRANSPARENCY
	}
#endif

	KviUserListEntry * pEntry = m_pListView->m_pTopItem;

	int iTheY = KVI_USERLIST_BORDER_WIDTH - m_iTopItemOffset;

	kvi_time_t curTime = kvi_unixTime();

	bool bShowIcons = KVI_OPTION_BOOL(KviOption_boolShowUserChannelIcons);
	bool bShowState = KVI_OPTION_BOOL(KviOption_boolShowUserChannelState);
	bool bShowGender = KVI_OPTION_BOOL(KviOption_boolDrawGenderIcons);

	while(pEntry && iTheY <= r.bottom())
	{
		int iBottom = iTheY + pEntry->m_iHeight;
		// iTheY is our top line
		// iTheX is our left corner
		// iBottom is our bottom line
		// iWidth is the width of the whole widget

		if(iBottom >= r.top())
		{
			QColor * pClrFore = 0;
			if(pEntry->m_bSelected)
			{
				QColor col = KVI_OPTION_COLOR(KviOption_colorUserListViewSelectionBackground);
				col.setAlpha(150);
				p.fillRect(0,iTheY,iWidth,pEntry->m_iHeight,col);
				pClrFore = &(KVI_OPTION_COLOR(KviOption_colorUserListViewSelectionForeground));
			} else if(KVI_OPTION_BOOL(KviOption_boolUseDifferentColorForOwnNick) && m_pListView->m_pKviWindow->connection())
			{
				if(pEntry->m_szNick==m_pListView->m_pKviWindow->connection()->currentNickName())
				{
					pClrFore = &(KVI_OPTION_COLOR(KviOption_colorUserListViewOwnForeground));
				}
			}

			if(!pClrFore)
			{
				// FIXME:
				//
				// this is slow... VERY slow when one has a lot of registered users.
				//   (this is NOT a simple lookup in the user db... it is a mask match)
				// if we REALLY need to use custom colors for regged users then
				// they should be updated ONCE and stored (cached) in the KviUserListEntry structure
				//
				if(m_pListView->m_pKviWindow->connection()->userDataBase()->haveCustomColor(pEntry->m_szNick))
				{
					pClrFore = m_pListView->m_pKviWindow->connection()->userDataBase()->customColor(pEntry->m_szNick);
				}

				if(!pClrFore)
				{
					if(pEntry->m_iFlags == 0)
					{
						pClrFore = &(KVI_OPTION_COLOR(KviOption_colorUserListViewNormalForeground));
					} else {
						pClrFore = &(KVI_OPTION_COLOR((pEntry->m_iFlags & KVI_USERFLAG_CHANOWNER) ? \
							KviOption_colorUserListViewChanOwnerForeground : ((pEntry->m_iFlags & KVI_USERFLAG_CHANADMIN) ? \
							KviOption_colorUserListViewChanAdminForeground : ((pEntry->m_iFlags & KVI_USERFLAG_OP) ? \
							KviOption_colorUserListViewOpForeground : ((pEntry->m_iFlags & KVI_USERFLAG_HALFOP) ? \
							KviOption_colorUserListViewHalfOpForeground : ((pEntry->m_iFlags & KVI_USERFLAG_VOICE) ? \
							KviOption_colorUserListViewVoiceForeground : KviOption_colorUserListViewUserOpForeground))))));
					}
				}
			}

			int iTheX = KVI_USERLIST_BORDER_WIDTH + 1;

			int iAvatarAndTextX = iTheX;

			if(bShowGender)
				iAvatarAndTextX += 11;
			if(bShowIcons)
				iAvatarAndTextX += 18;
			if(bShowState)
				iAvatarAndTextX += 11;

			if(KVI_OPTION_BOOL(KviOption_boolUserListViewDrawGrid))
			{
				// the grid
				switch(KVI_OPTION_UINT(KviOption_uintUserListViewGridType))
				{
					case KVI_USERLISTVIEW_GRIDTYPE_PLAINGRID:
					case KVI_USERLISTVIEW_GRIDTYPE_DOTGRID:
						p.setPen(QPen(KVI_OPTION_COLOR(KviOption_colorUserListViewGrid),0,
							(KVI_OPTION_UINT(KviOption_uintUserListViewGridType) == KVI_USERLISTVIEW_GRIDTYPE_DOTGRID) ? Qt::DotLine : Qt::SolidLine));
						p.drawLine(0,iBottom - 1,iWidth,iBottom - 1);
						if(bShowState || bShowIcons)
							p.drawLine(iAvatarAndTextX,iBottom - 1,iAvatarAndTextX,iTheY);
					break;
					default: // KVI_USERLISTVIEW_GRIDTYPE_3DGRID and KVI_USERLISTVIEW_GRIDTYPE_3DBUTTONS
						if(!pEntry->m_bSelected)
						{
							p.setPen(QPen(KVI_OPTION_COLOR(KviOption_colorUserListViewGrid),0 /*,QPen::DotLine*/));
							if((bShowState || bShowIcons) && (KVI_OPTION_UINT(KviOption_uintUserListViewGridType) == KVI_USERLISTVIEW_GRIDTYPE_3DGRID))
								p.drawLine(iAvatarAndTextX,iBottom - 1,iAvatarAndTextX,iTheY);
							p.setPen(palette().shadow().color());
							p.drawLine(0,iBottom - 1,iWidth,iBottom - 1);
							p.setPen(palette().light().color());
							p.drawLine(0,iTheY,iWidth,iTheY);
							iTheY--;
						}
						iTheY++;
					break;
				}
				iAvatarAndTextX += 3;
			} else {
				iAvatarAndTextX += 1;
			}

			if(pEntry->globalData()->isAway())
			{
				QRgb rgb2 = pClrFore->rgb();
				QRgb rgb1 = KVI_OPTION_COLOR(KviOption_colorUserListViewAwayForeground).rgb();
				p.setPen(QColor(
					((qRed(rgb1)*2) + qRed(rgb2)) / 3,
					((qGreen(rgb1)*2) + qGreen(rgb2)) / 3,
					((qBlue(rgb1)*2) + qBlue(rgb2)) / 3)
				);
			} else {
				p.setPen(*pClrFore);
			}
			iTheY += 2;

			if(KVI_OPTION_BOOL(KviOption_boolShowAvatarsInUserlist))//G&N  2005
			{
				KviAvatar * pAv = pEntry->m_pGlobalData->avatar();
				if(pAv && KVI_OPTION_UINT(KviOption_uintAvatarScaleWidth) && KVI_OPTION_UINT(KviOption_uintAvatarScaleHeight))
				{
					QPixmap * pPix;
					if( KVI_OPTION_BOOL(KviOption_boolScaleAvatars) &&
						(
							!KVI_OPTION_BOOL(KviOption_boolDoNotStretchAvatars) ||
							(pAv->size().width() > KVI_OPTION_UINT(KviOption_uintAvatarScaleWidth)) ||
							(pAv->size().height() > KVI_OPTION_UINT(KviOption_uintAvatarScaleHeight))
						)
					) {
						pPix = pAv->forSize(
								KVI_OPTION_UINT(KviOption_uintAvatarScaleWidth),
								KVI_OPTION_UINT(KviOption_uintAvatarScaleHeight)
								)->pixmap();
					} else {
						pPix = pAv->pixmap();
					}
					//debug("Pixmap: null=%i;height=%i;width=%i",pix.isNull(),pix.height(),pix.width());
					p.drawPixmap(iAvatarAndTextX,iTheY,*pPix);
					iTheY += pPix->height() + 1;
				}
			}

			if(bShowGender)
			{
				if(pEntry->globalData()->gender()!=KviIrcUserEntry::Unknown)
				{
					QPixmap * pIco = g_pIconManager->getBigIcon((pEntry->globalData()->gender()==KviIrcUserEntry::Male) ? "kvi_icon_male.png" : "kvi_icon_female.png");
					p.drawPixmap(iTheX,iTheY+(m_pListView->m_iFontHeight-11)/2,*pIco);
				}

				if(pEntry->globalData()->isBot())
				{
					QPixmap * pIco = g_pIconManager->getBigIcon("kvi_icon_bot.png");
					p.drawPixmap(iTheX,iTheY+(m_pListView->m_iFontHeight-11)/2,*pIco);
				}
				iTheX +=11;
			}

			if(bShowState)
			{
				if(pEntry->m_lastActionTime)
				{
					// the g_pUserChanStatePixmap is 36 x 80 pixels
					// divided into 6 rows of 5 pixmaps
					// row 0 is hot , row 5 is cold
					// left is most active , right is least active
					// e->m_iTemperature is a signed short , negative values are cold
					// e->m_lastActionTime is the time of the last action (eventually 0 , if not known)
					// 6 bit right shift is an aprox division for 64 : really aprox minutes
					unsigned int uTimeDiff = (((unsigned int)(curTime - pEntry->m_lastActionTime)) >> 6);
					if(uTimeDiff < 16)
					{
						//p.drawRect(iTheX,iTheY + 2,10,e->m_iHeight - 4);
						static int iXOffTable[16] =
						{
							0  , 8  , 16 , 16 ,
							24 , 24 , 24 , 24 ,
							32 , 32 , 32 , 32 ,
							32 , 32 , 32 , 32
						};
						// the temperature now
						// temp > 100 is hot (offset y = 0)
						// temp < -100 is cold (offset y = 80)
						// temp > 30 is half-hot (offset y = 16)
						// temp < -30 is half-cold (offset y = 64)
						// temp > 0 is a-bit-hot (offset y = 32)
						// temp < 0 is a-bit-cold (offset y = 48)

						if(pEntry->m_iTemperature > KVI_MID_TEMPERATURE)
						{
							if(pEntry->m_iTemperature > KVI_HALF_HOT_TEMPERATURE)
							{
								if(pEntry->m_iTemperature > KVI_HOT_TEMPERATURE)
								{
									// hot
									p.drawPixmap(iTheX,iTheY,*g_pUserChanStatePixmap,iXOffTable[uTimeDiff],0,8,16);
								} else {
									// half-hot
									p.drawPixmap(iTheX,iTheY,*g_pUserChanStatePixmap,iXOffTable[uTimeDiff],16,8,16);
								}
							} else {
								// bit-hot
								p.drawPixmap(iTheX,iTheY,*g_pUserChanStatePixmap,iXOffTable[uTimeDiff],32,8,16);
							}
						} else {
							if(pEntry->m_iTemperature < KVI_HALF_COLD_TEMPERATURE)
							{
								if(pEntry->m_iTemperature < KVI_COLD_TEMPERATURE)
								{
									// cold
									p.drawPixmap(iTheX,iTheY,*g_pUserChanStatePixmap,iXOffTable[uTimeDiff],80,8,16);
								} else {
									// half-cold
									p.drawPixmap(iTheX,iTheY,*g_pUserChanStatePixmap,iXOffTable[uTimeDiff],64,8,16);
								}
							} else {
								// bit-cold
								p.drawPixmap(iTheX,iTheY,*g_pUserChanStatePixmap,iXOffTable[uTimeDiff],48,8,16);
							}
						}
					}
				}
				iTheX += 11;
			}

			if(bShowIcons)
			{
				//p.drawRect(iTheX,iTheY + 2,18,e->m_iHeight - 4);
				if(pEntry->m_iFlags != 0)
				{
					QPixmap * pIco = g_pIconManager->getSmallIcon( \
											pEntry->globalData()->isAway() ? \
												( \
													(pEntry->m_iFlags & KVI_USERFLAG_CHANOWNER) ? \
													KVI_SMALLICON_CHANOWNERAWAY : ((pEntry->m_iFlags & KVI_USERFLAG_CHANADMIN) ? \
													KVI_SMALLICON_CHANADMINAWAY : ((pEntry->m_iFlags & KVI_USERFLAG_OP) ? \
													KVI_SMALLICON_OPAWAY : ((pEntry->m_iFlags & KVI_USERFLAG_HALFOP) ? \
													KVI_SMALLICON_HALFOPAWAY : ((pEntry->m_iFlags & KVI_USERFLAG_VOICE) ? \
													KVI_SMALLICON_VOICEAWAY : KVI_SMALLICON_USEROPAWAY))))
												) \
											: \
												( \
													(pEntry->m_iFlags & KVI_USERFLAG_CHANOWNER) ? \
													KVI_SMALLICON_CHANOWNER : ((pEntry->m_iFlags & KVI_USERFLAG_CHANADMIN) ? \
													KVI_SMALLICON_CHANADMIN : ((pEntry->m_iFlags & KVI_USERFLAG_OP) ? \
													KVI_SMALLICON_OP : ((pEntry->m_iFlags & KVI_USERFLAG_HALFOP) ? \
													KVI_SMALLICON_HALFOP : ((pEntry->m_iFlags & KVI_USERFLAG_VOICE) ? \
													KVI_SMALLICON_VOICE : KVI_SMALLICON_USEROP)))) \
												) \
										);
					p.drawPixmap(iTheX,iTheY+(fm.lineSpacing()-16/*size of small icon*/)/2,*pIco);
				}
				iTheX +=18;
				p.drawText(iAvatarAndTextX,iTheY,iWidth - iTheX,fm.lineSpacing(),Qt::AlignLeft|Qt::AlignVCenter,pEntry->m_szNick);
			} else {

				char cFlag = m_pListView->getUserFlag(pEntry);
				if(cFlag)
				{
					QString szTmp = QChar(cFlag);
					szTmp += pEntry->m_szNick;
					p.drawText(iAvatarAndTextX,iTheY,iWidth - iTheX,fm.lineSpacing(),Qt::AlignLeft|Qt::AlignVCenter,szTmp);
				} else {
					p.drawText(iAvatarAndTextX,iTheY,iWidth - iTheX,fm.lineSpacing(),Qt::AlignLeft|Qt::AlignVCenter,pEntry->m_szNick);
				}
			}
		}

		iTheY = iBottom;
		pEntry = pEntry->m_pNext;
	}

	//we really do not need any self-draw borders.
	//if we will need it, we will draw a better one with system style

	//p.setPen(colorGroup().dark());
	//p.drawLine(0,0,wdth,0);
	//p.drawLine(0,0,0,height());
	//p.setPen(colorGroup().light());
	//p.drawLine(1,height()-1,wdth,height()-1);
	//p.drawLine(wdth - 1,1,wdth - 1,height());

	QPainter qt4SuxBecauseOfThisAdditionalPainter(this);
	qt4SuxBecauseOfThisAdditionalPainter.drawPixmap(r.left(),r.top(),r.width(),r.height(),*pMemBuffer,r.left(),r.top(),r.width(),r.height());
}

void KviUserListViewArea::resizeEvent(QResizeEvent * e)
{
	int iScr = m_pScrollBar->sizeHint().width();
	m_pScrollBar->setGeometry(width() - iScr,0,iScr,height());
	m_pScrollBar->setPageStep(height());
	m_pScrollBar->setSingleStep(m_pListView->m_iFontHeight - 1);
}

void KviUserListViewArea::mousePressEvent(QMouseEvent * e)
{
	setFocus();
	if(e->button() & Qt::LeftButton)
	{
		KviUserListEntry * pEntry = m_pListView->itemAt(e->pos());
		if(pEntry)
		{
			if(e->modifiers() & Qt::ShiftModifier)
			{
				// Multiselect mode
				if(!pEntry->m_bSelected)
					m_pListView->m_iSelectedCount++;
				pEntry->m_bSelected = true;

				if(m_pListView->m_iSelectedCount == 1)
					g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,true);
				update();
			} else if(e->modifiers() & Qt::ControlModifier)
			{
				// Invert mode
				if(!pEntry->m_bSelected)
					m_pListView->m_iSelectedCount++;
				else m_pListView->m_iSelectedCount--;

				pEntry->m_bSelected = !pEntry->m_bSelected;
				if(m_pListView->m_iSelectedCount == 0)
					g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,false);
				else if(m_pListView->m_iSelectedCount == 1)
					g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,true);
				update();
			} else {
				// Single select mode
				bool bThereWasSelection = false;
				if(m_pListView->m_iSelectedCount > 0)
				{
					KviUserListEntry * pAux = m_pListView->m_pHeadItem;
					while(pAux)
					{
						pAux->m_bSelected = false;
						pAux = pAux->m_pNext;
					}
					bThereWasSelection = true;
				}
				pEntry->m_bSelected = true;
				m_pListView->m_iSelectedCount = 1;

				if(!bThereWasSelection)
					g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,true);
				update();
			}
		}
		m_pLastEntryUnderMouse = pEntry;
	} else if(e->button() & Qt::RightButton)
	{
		KviUserListEntry * pEntry = m_pListView->itemAt(e->pos());
		if(pEntry)
		{
			if(!pEntry->m_bSelected){
				pEntry->m_bSelected = true;
				m_pListView->m_iSelectedCount=1;

				KviPointerHashTableIterator<QString,KviUserListEntry> it(*(m_pListView->m_pEntryDict));
				while(it.current())
				{
					if(it.current() != pEntry)
						((KviUserListEntry *)it.current())->m_bSelected = false;
					++it;
				}
			}
			if(m_pListView->m_iSelectedCount == 1)
				g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,true);
			update();
		}
		m_pListView->emitRightClick();
	}
}

void KviUserListViewArea::keyPressEvent(QKeyEvent * e)
{
	if(e->key() == Qt::Key_Escape)
	{
		if(m_pListView->m_pKviWindow->input())
			((QWidget*)(m_pListView->m_pKviWindow->input()))->setFocus();
	} else if(
		e->modifiers().testFlag(Qt::ControlModifier) &&
		e->key() == 'a'
		)
	{
		KviUserListEntry * pAux = m_pListView->m_pHeadItem;
		while(pAux)
		{
			pAux->m_bSelected = true;
			pAux = pAux->m_pNext;
		}

		g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,true);
		update();
	} else {
		QString szKey = e->text();
		if(!szKey.isEmpty())
		{
			KviUserListEntry * pNick = 0;
			KviUserListEntry * pAux = m_pListView->m_pHeadItem;
			while(pAux)
			{
				//debug("%s %s %i %s %i",__FILE__,__FUNCTION__,__LINE__,aux->nick().toUtf8().data(),aux->nick().find(szKey,0,0));
				if(pAux->nick().indexOf(szKey,0,Qt::CaseInsensitive)==0)
				{
					pNick = pAux;
					break;
				}
				pAux = pAux->m_pNext;
			}

			if(pNick)
			{
				bool bThereWasSelection = false;
				if(m_pListView->m_iSelectedCount > 0)
				{
					pAux = m_pListView->m_pHeadItem;
					while(pAux)
					{
						pAux->m_bSelected = false;
						pAux = pAux->m_pNext;
					}
					bThereWasSelection = true;
				}
				pNick->m_bSelected = true;
				m_pListView->m_iSelectedCount = 1;

				if(!bThereWasSelection)
					g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,true);
				update();
			}
		}
	}
}

void KviUserListViewArea::mouseDoubleClickEvent(QMouseEvent * e)
{
	m_pListView->emitDoubleClick();
}

void KviUserListViewArea::mouseMoveEvent(QMouseEvent * e)
{
	if(e->modifiers() & Qt::LeftButton)
	{
		KviUserListEntry * pEntry = m_pListView->itemAt(e->pos());
		if(pEntry && (pEntry != m_pLastEntryUnderMouse))
		{
			if(e->modifiers() & Qt::ControlModifier)
			{
				if(pEntry->m_bSelected)
					m_pListView->m_iSelectedCount--;
				else m_pListView->m_iSelectedCount++;

				pEntry->m_bSelected = ! pEntry->m_bSelected;

				if(m_pListView->m_iSelectedCount == 0)
					g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,false);
				else if(m_pListView->m_iSelectedCount == 1)
					g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,true);
			} else {
				if(!pEntry->m_bSelected)
					m_pListView->m_iSelectedCount++;
				pEntry->m_bSelected = true;

				if(m_pListView->m_iSelectedCount == 1)
					g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,true);
			}
			update();
			m_pLastEntryUnderMouse = pEntry;
		} else {
			// out of the widget ?
			if(pEntry == m_pLastEntryUnderMouse)
				return;
			if(e->pos().y() < KVI_USERLIST_BORDER_WIDTH)
			{
				KviUserListEntry * pTop = m_pListView->m_pTopItem;
				if(pTop)
				{
					m_pScrollBar->setValue(m_pScrollBar->value() - pTop->m_iHeight);
					if(m_pListView->m_pTopItem != pTop)
					{
						if(e->modifiers() & Qt::ControlModifier)
						{
							if(m_pListView->m_pTopItem->m_bSelected)
								m_pListView->m_iSelectedCount--;
							else m_pListView->m_iSelectedCount++;

							m_pListView->m_pTopItem->m_bSelected = ! m_pListView->m_pTopItem->m_bSelected;

							if(m_pListView->m_iSelectedCount == 0)
								g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,false);
							else if(m_pListView->m_iSelectedCount == 1)
								g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,true);
						} else {
							if(!m_pListView->m_pTopItem->m_bSelected)
								m_pListView->m_iSelectedCount++;
							m_pListView->m_pTopItem->m_bSelected = true;

							if(m_pListView->m_iSelectedCount == 1)
								g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,true);
						}
						update();
					}
				}
				m_pLastEntryUnderMouse = pTop;
			} else if(e->pos().y() > (height() - KVI_USERLIST_BORDER_WIDTH))
			{
				KviUserListEntry * pBottom = m_pListView->m_pTopItem;
				if(pBottom)
				{
					int iTheY = KVI_USERLIST_BORDER_WIDTH - m_iTopItemOffset;
					while(pBottom && (iTheY < height()))
					{
						iTheY += pBottom->m_iHeight;
						pBottom = pBottom->m_pNext;
					}

					if(!pBottom)
						pBottom = m_pListView->m_pTailItem;
					if(pBottom)
					{
						m_pScrollBar->setValue(m_pScrollBar->value() + pBottom->m_iHeight);

						if(pBottom != m_pLastEntryUnderMouse)
						{
							if(e->modifiers() & Qt::ControlModifier)
							{
								if(pBottom->m_bSelected)
	m_pListView->m_iSelectedCount--;
								else m_pListView->m_iSelectedCount++;
								pBottom->m_bSelected = ! pBottom->m_bSelected;
								if(m_pListView->m_iSelectedCount == 0)
									g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,false);
								else if(m_pListView->m_iSelectedCount == 1)						g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,true);
							} else {
								if(!pBottom->m_bSelected)
									m_pListView->m_iSelectedCount++;
								pBottom->m_bSelected = true;
								if(m_pListView->m_iSelectedCount == 1)
									g_pFrame->childWindowSelectionStateChange(m_pListView->m_pKviWindow,true);
							}
							update();
						}
					}
				}
				m_pLastEntryUnderMouse = pBottom;
			} else m_pLastEntryUnderMouse = 0;
		}
	}
}

void KviUserListViewArea::mouseReleaseEvent(QMouseEvent * e)
{
	m_pLastEntryUnderMouse = 0;
}

void KviUserListViewArea::wheelEvent(QWheelEvent * e)
{
	static bool bHere = false; // Qt4(<= 4.2.2) has a nasty bug that makes the re-sent wheelEvent to cause infinite recursion
	if(bHere)
		return;
	bHere = true;
	g_pApp->sendEvent(m_pScrollBar,e);
	bHere = false;
}

#ifndef COMPILE_USE_STANDALONE_MOC_SOURCES
#include "kvi_userlistview.moc"
#endif //!COMPILE_USE_STANDALONE_MOC_SOURCES
