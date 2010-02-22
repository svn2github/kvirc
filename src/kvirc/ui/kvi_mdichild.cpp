//=============================================================================
//
//   File : kvi_mdichild.cpp
//   Creation date : Wed Jun 21 2000 17:35:45 by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2000 Szymon Stefanek (pragma at kvirc dot net)
//   Copyright (C) 2008 TheXception (kvirc at thexception dot net)
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

/**
* \file kvi_mdichild.cpp
* \brief MDI subwindow stuff
*/

#define _KVI_DEBUG_CHECK_RANGE_
#include "kvi_debug.h"
#include "kvi_mdichild.h"
#include "kvi_mdimanager.h"
#include "kvi_string.h"
#include "kvi_locale.h"
#include "kvi_options.h"
#include "kvi_settings.h"
#include "kvi_iconmanager.h"
#include "kvi_window.h"
#include "kvi_tal_popupmenu.h"
#include "kvi_ircconnection.h"
#include "kvi_ircconnectiontarget.h"
#include "kvi_irccontext.h"
#include "kvi_network.h"

#ifdef COMPILE_ON_MAC
	#include "kvi_app.h"  //Needed for g_pApp
#endif

#include <QTimer>

#ifdef COMPILE_PSEUDO_TRANSPARENCY
	extern QPixmap * g_pShadedChildGlobalDesktopBackground;
#endif

KviMdiChild::KviMdiChild(KviMdiManager * par, const char * name)
: QMdiSubWindow(par)
{
	setObjectName(name ? name : "mdi_child");

	m_pManager = par;
	m_pClient = 0;

	m_restoredGeometry   = QRect(10,10,320,240);

	connect(systemMenu(), SIGNAL(aboutToShow()), this, SLOT(updateSystemPopup()));
	connect(this, SIGNAL(windowStateChanged(Qt::WindowStates, Qt::WindowStates)), this, SLOT(windowStateChangedEvent(Qt::WindowStates, Qt::WindowStates)));

	QTimer::singleShot( 0, this, SLOT(transparencyWorkaround()));
}

KviMdiChild::~KviMdiChild()
{
	if(m_pClient)
	{
		delete m_pClient;
		m_pClient=0;
	}
	m_pManager->focusPreviousTopChild();
}

void KviMdiChild::transparencyWorkaround()
{
	setAutoFillBackground(true);
}
	
void KviMdiChild::setRestoredGeometry(const QRect &r)
{
	m_restoredGeometry = r;
	setGeometry(r);
}

void KviMdiChild::closeEvent(QCloseEvent * e)
{
	//we let kviwindow handle the process
	KviWindow * pClient = (KviWindow*) widget();
	if(pClient)
	{
		e->ignore();
		pClient->delayedClose();
	}
}

QRect KviMdiChild::restoredGeometry()
{
	return geometry();
}

KviMdiChild::MdiChildState KviMdiChild::state()
{
	if(isMinimized())
		return Minimized;
	if(isMaximized())
		return Maximized;
	return Normal;
}

void KviMdiChild::setIcon(QPixmap pix)
{
	m_pIcon = pix;
	QMdiSubWindow::setWindowIcon(QIcon(pix));
}

const QPixmap * KviMdiChild::icon()
{
	return &m_pIcon;
}

void KviMdiChild::setWindowTitle(const QString & plain)
{
	m_szPlainCaption = plain;
	QMdiSubWindow::setWindowTitle(plain);
}

void KviMdiChild::windowStateChangedEvent(Qt::WindowStates oldState, Qt::WindowStates newState)
{
// 	qDebug("%s %d => %d", m_szPlainCaption.toUtf8().data(), (int) oldState, (int) newState);
	Qt::WindowStates diffState = oldState ^ newState;

	if(diffState.testFlag(Qt::WindowMinimized))
	{
		//minimized or unminimized
		updateCaption();
	}
}

void KviMdiChild::restore()
{
	if(isMinimized())
	{
		if(isMaximized())
			showMaximized();
		else showNormal();
		return;
	}

	if(isMaximized())
	{
		setWindowState(windowState() & ~Qt::WindowMaximized);
		showNormal();
		return;
	}

	if(!isVisible())
	{
		show();
		return;
	}
}

void KviMdiChild::maximize()
{
	showMaximized();
}

void KviMdiChild::minimize()
{
	showMinimized();
}

void KviMdiChild::updateCaption()
{
	if(m_pClient)
		if(m_pClient->inherits("KviWindow"))
		{
			((KviWindow*)m_pClient)->updateCaption();
		}
}

void KviMdiChild::moveEvent(QMoveEvent *e)
{
#ifdef COMPILE_PSEUDO_TRANSPARENCY
	//this is not needed for real transparency using compositing
	if(m_pClient && g_pShadedChildGlobalDesktopBackground)
	{
		if(m_pClient->inherits("KviWindow")) // actually this is always the case
		{
			((KviWindow *)m_pClient)->updateBackgrounds();
		}
	}
#endif
	QMdiSubWindow::moveEvent(e);
}

void KviMdiChild::setClient(QWidget * w)
{
	__range_valid(m_pClient==0);
	__range_valid(w!=0);

	m_pClient = w;
	setWidget(w);
        QString tmp1;
        if(w->inherits("KviWindow"))
        {
            KviIrcConnection * pConnection=((KviWindow*)w)->connection();
            if (pConnection) tmp1.append(pConnection->target()->network()->name()+"_");
        }

        KviStr tmp(KviStr::Format,"mdi_child_%s%s",tmp1.toUtf8().data(),w->objectName().toUtf8().data());
	setObjectName(tmp.ptr());
}

void KviMdiChild::unsetClient()
{
	__range_valid(m_pClient!=0);
	if(!m_pClient)return;
	setWidget(0);
	m_pClient = 0;
	setObjectName("mdi_child");
}

void KviMdiChild::activate()
{
	if(m_pManager->topChild() != this)
		m_pManager->setTopChild(this);
}

void KviMdiChild::updateSystemPopup()
{
	if(m_pClient)
		if(m_pClient->inherits("KviWindow"))
		{
			systemMenu()->clear();
			QMenu * tmp = ((KviWindow*) m_pClient)->generatePopup();
			if (tmp)
			{
				systemMenu()->addActions(tmp->actions());
			}
		}
}
