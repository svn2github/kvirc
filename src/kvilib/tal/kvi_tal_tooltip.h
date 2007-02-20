#ifndef _KVI_TAL_TOOLTIP_H_
#define _KVI_TAL_TOOLTIP_H_

//=============================================================================
//
//   File : kvi_tal_tooltip.h
//   Creation date : Mon Jan 22 2007 11:25:08 by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2007 Szymon Stefanek (pragma at kvirc dot net)
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

#include "kvi_settings.h"

#include <qtooltip.h>
#include <qobject.h>

class KviTalToolTip;

// This is useful only with Qt4, but we put it here anyway
// to have both a consistent API and make moc happy
class KviTalToolTipHelper : public QObject
{
	friend class KviTalToolTip;
	Q_OBJECT
public:
	KviTalToolTipHelper(KviTalToolTip * pToolTip,QWidget * pWidget);
	~KviTalToolTipHelper();
protected:
	KviTalToolTip * m_pToolTip;
protected:
	virtual bool eventFilter(QObject * pObject,QEvent * pEvent);
	void toolTipDying();
};


class KVILIB_API KviTalToolTip
#ifndef COMPILE_USE_QT4
 : public QToolTip
#endif
{
	friend class KviTalToolTipHelper;
public:
	KviTalToolTip(QWidget * pParent);
	virtual ~KviTalToolTip();
protected:
#ifdef COMPILE_USE_QT4
	KviTalToolTipHelper * m_pHelper;
	QWidget * m_pParent;
#endif
public:
#ifdef COMPILE_USE_QT4
	static void add(QWidget * widget,const QString & text);
	static void remove(QWidget * widget);
	virtual void tip(const QRect & rect,const QString & text);
#endif
protected:
	virtual void maybeTip(const QPoint & p);
#ifdef COMPILE_USE_QT4
	void helperDying();
#endif
};

#endif // _KVI_TAL_TOOLTIP_H_
