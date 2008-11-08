//=============================================================================
//
//   File : kvi_customtoolbardescriptor.cpp
//   Creation date : Sun 05 Dec 2004 18:20:58 by Szymon Stefanek
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
//   Inc. ,59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//=============================================================================



#include "kvi_customtoolbardescriptor.h"
#include "kvi_customtoolbar.h"
#include "kvi_config.h"
#include "kvi_action.h"
#include "kvi_actionmanager.h"
#include "kvi_app.h"
#include "kvi_locale.h"
#include "kvi_kvs_script.h"
#include "kvi_window.h"

KviCustomToolBarDescriptor::KviCustomToolBarDescriptor(const QString &szId,const QString &szLabelCode)
{
	m_iInternalId = g_pApp->getGloballyUniqueId();
	m_szId = szId;
	m_pActions = new KviPointerList<QString>;
	m_pActions->setAutoDelete(true);
	m_pToolBar = 0;
	m_bVisibleAtStartup = false;
	createLabelScript(szLabelCode);
}

KviCustomToolBarDescriptor::~KviCustomToolBarDescriptor()
{
	if(m_pToolBar)delete m_pToolBar; // first kill the toolbar (so eventually leave it sync the actions)
	delete m_pActions; // THEN kill the actions
	delete m_pLabelScript;
}

void KviCustomToolBarDescriptor::createLabelScript(const QString &szLabelCode)
{
	QString szTmp = "toolbar::";
	szTmp += m_szId;
	szTmp += "::label";
	m_pLabelScript = new KviKvsScript(szTmp,szLabelCode,KviKvsScript::Parameter);
}

const QString & KviCustomToolBarDescriptor::label()
{
	if(!m_pLabelScript->run(g_pActiveWindow,0,m_szParsedLabel))m_szParsedLabel = m_pLabelScript->code();
	return m_szParsedLabel;
}

const QString & KviCustomToolBarDescriptor::labelCode()
{
	return m_pLabelScript->code();
}

bool KviCustomToolBarDescriptor::addAction(const QString &szAction)
{
	KviAction *a = KviActionManager::instance()->getAction(szAction);
	if(a)
	{
		m_pActions->append(new QString(szAction));
		if(m_pToolBar)a->addToCustomToolBar(m_pToolBar);
		return true;
	}
	return false;
}

bool KviCustomToolBarDescriptor::removeAction(unsigned int iAction)
{
	if(iAction >= m_pActions->count())return false;
	if(!m_pActions->remove(iAction))return false;
	if(m_pToolBar)
	{
		// need to refill it :/
		m_pToolBar->clear();
		fillToolBar();
	}
	return true;
}

bool KviCustomToolBarDescriptor::removeAction(const QString &szAction)
{
	for(QString *s = m_pActions->first();s;s = m_pActions->next())
	{
		if(*s == szAction)
		{
			m_pActions->removeRef(s);
			if(m_pToolBar)
			{
				// need to refill it :/
				m_pToolBar->clear();
				fillToolBar();
			}
			return true;
		}
	}
	return false;
}

void KviCustomToolBarDescriptor::updateToolBar()
{
	if(!m_pToolBar)return;
	m_pToolBar->clear();
	fillToolBar();
}


void KviCustomToolBarDescriptor::fillToolBar()
{
	if(m_pActions->count() == 0)
	{
		// force layout of the toolbar
		QApplication::postEvent(m_pToolBar,new QEvent(QEvent::LayoutRequest));
	} else {
		KviPointerList<QString> dying;
		dying.setAutoDelete(false);
		for(QString *p = m_pActions->first();p;p = m_pActions->next())
		{
			KviAction *a = KviActionManager::instance()->getAction(*p);
			if(a)a->addToCustomToolBar(m_pToolBar);
			else dying.append(p);
		}
		for(QString *d = dying.first();d;d = dying.next())
			m_pActions->removeRef(d);
	}
}

KviCustomToolBar * KviCustomToolBarDescriptor::createToolBar()
{
	if(!m_pToolBar)
	{
		QByteArray dd = id().toUtf8();
		m_pToolBar = new KviCustomToolBar(this,label(),Qt::TopToolBarArea,false,dd.data());
		fillToolBar();
	}
	return m_pToolBar;
}

void KviCustomToolBarDescriptor::registerToolBar(KviCustomToolBar * t)
{
	m_pToolBar = t;
}

void KviCustomToolBarDescriptor::unregisterToolBar(KviCustomToolBar *)
{
	m_pToolBar = 0;
}

void KviCustomToolBarDescriptor::rename(const QString &szNewLabelCode)
{
	delete m_pLabelScript;
	createLabelScript(szNewLabelCode);
	if(m_pToolBar)m_pToolBar->setWindowTitle(label());
}

void KviCustomToolBarDescriptor::clear()
{
	m_pActions->clear();
	if(m_pToolBar)
	{
		delete m_pToolBar;
		m_pToolBar = 0;
	}
}

bool KviCustomToolBarDescriptor::load(KviConfig * cfg)
{
	m_pActions->clear();
	m_szId = cfg->group();
	QString szLabelCode = cfg->readQStringEntry("Label");
	if(szLabelCode.isEmpty())szLabelCode = "$tr(Unnamed)";
	delete m_pLabelScript;
	createLabelScript(szLabelCode);
	m_szIconId = cfg->readQStringEntry("IconId","");
	unsigned int n = cfg->readUIntEntry("Count",0);
	QString tmp;
	for(unsigned int i=0;i<n;i++)
	{
		tmp.setNum(i);
		QString * p = new QString(cfg->readQStringEntry(tmp));
		if(p->isEmpty())delete p;
		else m_pActions->append(p);
	}
	return true;
}

void KviCustomToolBarDescriptor::save(KviConfig * cfg)
{
	cfg->setGroup(m_szId);
	cfg->writeEntry("Label",m_pLabelScript->code());
	cfg->writeEntry("Count",m_pActions->count());
	if(!m_szIconId.isEmpty())cfg->writeEntry("IconId",m_szIconId);
	int i = 0;
	QString tmp;
	for(QString * p = m_pActions->first();p;p = m_pActions->next())
	{
		tmp.setNum(i);
		cfg->writeEntry(tmp,*p);
		i++;
	}
}

