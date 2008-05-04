//=============================================================================
//
//   File : kvi_action.cpp
//   Created on Sun 21 Nov 2004 03:36:34 by Szymon Stefanek
//
//   This file is part of the KVIrc IRC Client distribution
//   Copyright (C) 2004 Szymon Stefanek <pragma at kvirc dot net>
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

#define __KVIRC__

#include "kvi_action.h"
#include "kvi_customtoolbar.h"
#include "kvi_frame.h"
#include "kvi_irccontext.h"
#include "kvi_ircconnection.h"
#include "kvi_iconmanager.h"
#include "kvi_app.h"
#include "kvi_window.h"
#include "kvi_channel.h"
#include "kvi_console.h"
#include "kvi_query.h"
#include "kvi_styled_controls.h"
#include "kvi_tal_popupmenu.h"

#include <QToolButton>

KviAction::~KviAction()
{
	if(m_pWidgetList)
	{
		for(QWidget * b = m_pWidgetList->first();b;b = m_pWidgetList->next())
			disconnect(b,SIGNAL(destroyed()),this,SLOT(widgetDestroyed()));
		m_pWidgetList->setAutoDelete(true);
		delete m_pWidgetList;
	}
}

const QString & KviAction::visibleName()
{
	return m_szVisibleName;
}

const QString & KviAction::description()
{
	return m_szDescription;
}

bool KviAction::isKviUserActionNeverOverrideThis()
{
	return false;
}

void KviAction::registerAccelerator()
{
	if(!m_szKeySequence.isEmpty())
		m_iAccelId = g_pFrame->registerAccelerator(m_szKeySequence,this,SLOT(activate()));
}

void KviAction::unregisterAccelerator()
{
	if(m_iAccelId != 0)
	{
		if(g_pFrame)
			g_pFrame->unregisterAccelerator(m_iAccelId);
		m_iAccelId = 0;
	}
}

void KviAction::setEnabled(bool bEnabled)
{
	if(bEnabled)
		m_uInternalFlags |= KVI_ACTION_FLAG_ENABLED;
	else
		m_uInternalFlags &= ~KVI_ACTION_FLAG_ENABLED;
	
	if(m_pWidgetList)
	{
		if(bEnabled)
		{
			for(QWidget * t = m_pWidgetList->first();t;t = m_pWidgetList->next())
				if(!t->isEnabled())t->setEnabled(true);
		} else {
			for(QWidget * t = m_pWidgetList->first();t;t = m_pWidgetList->next())
				if(t->isEnabled())t->setEnabled(false);
		}
	}
}

int KviAction::validateFlags(int iFlagsToValidate)
{
	if(iFlagsToValidate & EnableAtLogin)
		iFlagsToValidate |= NeedsConnection;
	if(iFlagsToValidate & NeedsConnection)
		iFlagsToValidate |= NeedsContext;
	if(iFlagsToValidate & WindowOnlyIfUsersSelected)
	{
		if(!(iFlagsToValidate & InternalWindowMask))
			iFlagsToValidate &= ~WindowOnlyIfUsersSelected;
	}
	return iFlagsToValidate;
}

QPixmap * KviAction::smallIcon()
{
	if(m_iSmallIconId != 0)return g_pIconManager->getSmallIcon(m_iSmallIconId);
	return 0;
}

QPixmap * KviAction::bigIcon()
{
	return g_pIconManager->getBigIcon(m_szBigIcon);
}

void KviAction::setup()
{
	connect(g_pApp,SIGNAL(reloadImages()),this,SLOT(reloadImages()));
	if(m_uFlags & InternalWindowMask)
	{
		connect(g_pFrame,SIGNAL(activeWindowChanged()),this,SLOT(activeWindowChanged()));
		if((m_uFlags & WindowOnlyIfUsersSelected) && (m_uFlags & (WindowChannel | WindowConsole | WindowQuery)))
			connect(g_pFrame,SIGNAL(activeWindowSelectionStateChanged(bool)),this,SLOT(activeWindowSelectionStateChanged(bool)));
		activeWindowChanged();
	} else {
		if(m_uFlags & NeedsConnection)
		{
			connect(g_pFrame,SIGNAL(activeContextChanged()),this,SLOT(activeContextChanged()));
			connect(g_pFrame,SIGNAL(activeContextStateChanged()),this,SLOT(activeContextStateChanged()));
			KviIrcContext * c = g_pFrame->activeContext();
			if(!c)setEnabled(false);
			else {
				switch(c->state())
				{
					case KviIrcContext::LoggingIn:
						setEnabled(m_uFlags & EnableAtLogin);
					break;
					case KviIrcContext::Connected:
						setEnabled(true);
					break;
					default:
						setEnabled(false);
					break;
				}
			}
		} else {
			if(m_uFlags & NeedsContext)
			{
				connect(g_pFrame,SIGNAL(activeContextChanged()),this,SLOT(activeContextChanged()));
				if(!g_pFrame->activeContext())setEnabled(false);
				else setEnabled(true);
			}
		}
	}

	m_uInternalFlags |= KVI_ACTION_FLAG_SETUPDONE;
}


void KviAction::reloadImages()
{
	if(!m_pWidgetList)return;
	QPixmap * p = bigIcon();
	for(QWidget * b = m_pWidgetList->first();b;b = m_pWidgetList->next())
	{
		if(b->inherits("QToolButton"))
			((QToolButton *)b)->setIcon(p ? *p : QPixmap());
	}
}

void KviAction::activeWindowChanged()
{
	if(m_uFlags & NeedsContext)
	{
		if(!g_pFrame->activeContext())
		{
			if(isEnabled())setEnabled(false);
			return;
		}
	}
	
	if(m_uFlags & NeedsConnection)
	{
		switch(g_pFrame->activeContext()->state())
		{
			case KviIrcContext::LoggingIn:
				if(!(m_uFlags & EnableAtLogin))
				{
					if(isEnabled())setEnabled(false);
					return;
				}
			break;
			case KviIrcContext::Connected:
				// this is ok 
			break;
			default:
				if(isEnabled())setEnabled(false);
				return;
			break;
		}
	}

	if(!g_pActiveWindow)
	{
		if(isEnabled())setEnabled(false);
		return;
	}

	switch(g_pActiveWindow->type())
	{
		case KVI_WINDOW_TYPE_CONSOLE:
			if(m_uFlags & WindowConsole)
			{
				if(m_uFlags & WindowOnlyIfUsersSelected)
				{
					bool bEnabled = ((KviConsole *)g_pActiveWindow)->selectedCount() > 0;
					if(bEnabled != isEnabled())setEnabled(bEnabled);
				} else {
					if(!isEnabled())setEnabled(true);
				}
			} else {
				if(isEnabled())setEnabled(false);
			}
			break;
		case KVI_WINDOW_TYPE_CHANNEL:
			if(m_uFlags & WindowChannel)
			{
				if(m_uFlags & WindowOnlyIfUsersSelected)
				{
					bool bEnabled = ((KviChannel *)g_pActiveWindow)->selectedCount() > 0;
					if(bEnabled != isEnabled())setEnabled(bEnabled);
				} else {
					if(!isEnabled())setEnabled(true);
				}
			} else {
				if(isEnabled())setEnabled(false);
			}
			break;
		case KVI_WINDOW_TYPE_QUERY:
			if(m_uFlags & WindowQuery)
			{
				if(m_uFlags & WindowOnlyIfUsersSelected)
				{
					bool bEnabled = ((KviQuery *)g_pActiveWindow)->selectedCount() > 0;
					if(bEnabled != isEnabled())setEnabled(bEnabled);
				} else {
					if(!isEnabled())setEnabled(true);
				}
			} else {
				if(isEnabled())setEnabled(false);
			}
			break;
		case KVI_WINDOW_TYPE_DCCCHAT:
			if(m_uFlags & WindowDccChat)
			{
				if(!isEnabled())setEnabled(true);
			} else {
				if(isEnabled())setEnabled(false);
			}
			break;
		default:
			if(m_uFlags & InternalWindowMask)
				if(isEnabled())setEnabled(false);
			else
				if(!isEnabled())setEnabled(true);
			break;
	}
}


void KviAction::activeWindowSelectionStateChanged(bool bSelectedNow)
{
	// we jump here ONLY if m_uFlags & WindowOnlyIfUsersSelected
	// and thus also m_uFlags & InternalWindowMask
	switch(g_pActiveWindow->type())
	{
		case KVI_WINDOW_TYPE_CONSOLE:
			if(m_uFlags & WindowConsole)
			{
				if(bSelectedNow != isEnabled())setEnabled(bSelectedNow);
			} else {
				if(isEnabled())setEnabled(false);
			}
			break;
		case KVI_WINDOW_TYPE_CHANNEL:
			if(m_uFlags & WindowChannel)
			{
				if(bSelectedNow != isEnabled())setEnabled(bSelectedNow);
			} else {
				if(isEnabled())setEnabled(false);
			}
			break;
		case KVI_WINDOW_TYPE_QUERY:
			if(m_uFlags & WindowQuery)
			{
				if(bSelectedNow != isEnabled())setEnabled(bSelectedNow);
			} else {
				if(isEnabled())setEnabled(false);
			}
			break;
		case KVI_WINDOW_TYPE_DCCCHAT:
			if(m_uFlags & WindowDccChat)
			{
				if(!isEnabled())setEnabled(true);
			} else {
				if(isEnabled())setEnabled(false);
			}
			break;
		default:
			if(isEnabled())setEnabled(false);
			break;
	}
}


void KviAction::activeContextChanged()
{
	// works only if NeedsContext is specified!
	KviIrcContext * c = g_pFrame->activeContext();
	if(c)
	{
		if(m_uFlags & NeedsConnection)
			activeContextStateChanged();
		else
			if(!isEnabled())setEnabled(true);
	} else {
		if(isEnabled())setEnabled(false);
	}
}

void KviAction::activeContextStateChanged()
{
	KviIrcContext * c = g_pFrame->activeContext();
	if(c)
	{
		switch(c->state())
		{
			case KviIrcContext::Idle:
			case KviIrcContext::Connecting:
				if(isEnabled())setEnabled(false);
			break;
			case KviIrcContext::LoggingIn:
				if(m_uFlags & EnableAtLogin)
				{
					if(!isEnabled())setEnabled(true);
				} else {
					if(isEnabled())setEnabled(false);
				}
			break;
			case KviIrcContext::Connected:
				if(!isEnabled())setEnabled(true);
			break;
			default:
				if(isEnabled())setEnabled(false);
			break;
		}
	} else {
		if(isEnabled())setEnabled(false);
	}
}

bool KviAction::addToPopupMenu(KviTalPopupMenu *pMenu)
{
	if(!setupDone())setup();
	QPixmap * p = smallIcon();
	int id;
	QString t = visibleName();
	if(!m_szKeySequence.isEmpty())t += '\t' + m_szKeySequence;
	if(p)
	{
		id = pMenu->insertItem(*p,t,this,SLOT(activate()));
	} else {
		id = pMenu->insertItem(t,this,SLOT(activate()));
	}
	if(!isEnabled())pMenu->setItemEnabled(id,false);
	return true;
}

void KviAction::widgetDestroyed()
{
	if(!m_pWidgetList)return;
	QWidget * b = (QWidget *)sender();
	m_pWidgetList->removeRef(b);
}

void KviAction::registerWidget(QWidget * b)
{
	connect(b,SIGNAL(destroyed()),this,SLOT(widgetDestroyed()));
	if(!m_pWidgetList)
	{
		m_pWidgetList = new KviPointerList<QWidget>;
		m_pWidgetList->setAutoDelete(false);
	}
	m_pWidgetList->append(b);
}

QWidget * KviAction::addToCustomToolBar(KviCustomToolBar *pParentToolBar)
{
	if(!setupDone())setup();
	QPixmap * p = bigIcon();
	QToolButton * b = new KviStyledToolButton(
			p ? *p : QPixmap(),
			visibleName(),
			visibleName(),
			this,SLOT(activate()),pParentToolBar,
			m_szName.toUtf8().data());

	pParentToolBar->addWidget(b);
	if(!isEnabled())b->setEnabled(false);
	registerWidget(b);
	return b;
}

void KviAction::activate()
{
	debug ("emits activated in action");
	if(isEnabled())
		emit activated();
}
