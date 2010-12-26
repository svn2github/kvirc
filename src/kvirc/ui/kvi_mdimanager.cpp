//=============================================================================
//
//   File : kvi_mdimanager.cpp
//   Creation date : Wed Jun 21 2000 17:28:04 by Szymon Stefanek
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
* \file kvi_mdimanager.cpp
* \brief The MDI-manager
*/


#include "kvi_debug.h"
#include "kvi_settings.h"
#include "kvi_mdimanager.h"
#include "kvi_mdichild.h"
#include "KviLocale.h"
#include "kvi_options.h"
#include "kvi_iconmanager.h"
#include "kvi_frame.h"
#include "kvi_menubar.h"
#include "kvi_app.h"
#include "KviTalPopupMenu.h"
#include "KviTalHBox.h"

#include <QMenuBar>
#include <QLayout>
#include <QPainter>
#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QLabel>
#include <QToolButton>

#ifdef COMPILE_PSEUDO_TRANSPARENCY
	#include <QPixmap>
	extern QPixmap * g_pShadedParentGlobalDesktopBackground;
#endif

#include <math.h>


/**
* \def KVI_MDICHILD_MIN_WIDTH Defines the minimum width of the child
* \def KVI_MDICHILD_MIN_HEIGHT Defines the minimum height of the child
*/
#define KVI_MDICHILD_MIN_WIDTH 100
#define KVI_MDICHILD_MIN_HEIGHT 40

KviMdiManager::KviMdiManager(QWidget * parent,KviFrame * pFrm,const char *)
: QMdiArea(parent)
{
	setFrameShape(NoFrame);
	m_bInSDIMode = KVI_OPTION_BOOL(KviOption_boolMdiManagerInSdiMode);
	m_pFrm = pFrm;

	m_pWindowPopup = new KviTalPopupMenu(this);
	connect(m_pWindowPopup,SIGNAL(activated(int)),this,SLOT(menuActivated(int)));
	connect(m_pWindowPopup,SIGNAL(aboutToShow()),this,SLOT(fillWindowPopup()));
	m_pTileMethodPopup = new KviTalPopupMenu(this);
	connect(m_pTileMethodPopup,SIGNAL(activated(int)),this,SLOT(tileMethodMenuActivated(int)));
	connect(this, SIGNAL(subWindowActivated(QMdiSubWindow *)), this, SLOT(slotSubWindowActivated(QMdiSubWindow *)));
	
	setAutoFillBackground(false);
	viewport()->setAutoFillBackground(false);

	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

KviMdiManager::~KviMdiManager()
{
}

void KviMdiManager::paintEvent(QPaintEvent * e)
{
	QPainter p(viewport());

#ifdef COMPILE_PSEUDO_TRANSPARENCY
	if(KVI_OPTION_BOOL(KviOption_boolUseCompositingForTransparency) && g_pApp->supportsCompositing())
	{
		p.save();
		p.setCompositionMode(QPainter::CompositionMode_Source);
		QColor col=KVI_OPTION_COLOR(KviOption_colorGlobalTransparencyFade);
		col.setAlphaF((float)((float)KVI_OPTION_UINT(KviOption_uintGlobalTransparencyParentFadeFactor) / (float)100));
		p.fillRect(e->rect(), col);
		p.restore();
		return;
	} else if(g_pShadedParentGlobalDesktopBackground)
	{
		QPoint pnt = viewport()->mapTo(g_pFrame, e->rect().topLeft() + scrollBarsOffset());
		p.drawTiledPixmap(e->rect(),*(g_pShadedParentGlobalDesktopBackground), pnt);
		return;
	}
#endif
	if(KVI_OPTION_PIXMAP(KviOption_pixmapMdiBackground).pixmap())
	{
		QPoint pnt = viewport()->mapTo(g_pFrame, e->rect().topLeft() + scrollBarsOffset());
		p.drawTiledPixmap(e->rect(),*(KVI_OPTION_PIXMAP(KviOption_pixmapMdiBackground).pixmap()), pnt);
	} else {
		p.fillRect(e->rect(),KVI_OPTION_COLOR(KviOption_colorMdiBackground));
	}
}

void KviMdiManager::manageChild(KviMdiChild * lpC, bool, QRect *)
{
	addSubWindow((QMdiSubWindow*)lpC);

	if(!m_bInSDIMode)
		if(KVI_OPTION_BOOL(KviOption_boolAutoTileWindows))tile();
}

void KviMdiManager::showAndActivate(KviMdiChild * lpC)
{
	setTopChild(lpC);

	if(m_bInSDIMode)
	{
		lpC->showMaximized();
	} else {
		lpC->show();
		if(KVI_OPTION_BOOL(KviOption_boolAutoTileWindows))tile();
	}
	lpC->activate();
}

KviMdiChild * KviMdiManager::topChild()
{
	return (KviMdiChild*)activeSubWindow();
}

void KviMdiManager::setTopChild(KviMdiChild *lpC)
{
	setActiveSubWindow((QMdiSubWindow*) lpC);
}

void KviMdiManager::hideChild(KviMdiChild *lpC)
{
	focusPreviousTopChild(lpC);
	lpC->hide();
}

void KviMdiManager::destroyChild(KviMdiChild *lpC)
{
	removeSubWindow(lpC);
	delete lpC;
	
	if(!m_bInSDIMode)
	{
		if(KVI_OPTION_BOOL(KviOption_boolAutoTileWindows))
			tile();
	}
}

void KviMdiManager::setIsInSDIMode(bool bMode)
{
// 	qDebug("Sdi Mode %d", bMode);
	m_bInSDIMode = bMode;
	if(!m_bInSDIMode)
		if(KVI_OPTION_BOOL(KviOption_boolAutoTileWindows))tile();
};

void KviMdiManager::focusPreviousTopChild(KviMdiChild * pExcludeThis)
{
	KviMdiChild * lpC = NULL;

	//QList<QMdiSubWindow *> tmp = subWindowList(QMdiArea::ActivationHistoryOrder);
	QList<QMdiSubWindow *> tmp = subWindowList(QMdiArea::StackingOrder);
	QListIterator<QMdiSubWindow*> wl(tmp);
	wl.toBack();

	while(wl.hasPrevious())
	{
		QMdiSubWindow * pSubWindow = wl.previous();

		if(!pSubWindow->inherits("KviMdiChild"))
			continue;

		lpC = static_cast<KviMdiChild *>(pSubWindow);

		if(lpC == pExcludeThis)
			continue;

		if(!lpC->isVisible())
			continue;

		if(lpC->state() != KviMdiChild::Minimized)
			break;
	}

	if(!lpC)
		return;

	if(isInSDIMode())
		lpC->maximize();
	else {
		if(KVI_OPTION_BOOL(KviOption_boolAutoTileWindows))
			tile();
	}
	lpC->raise();
	if(!lpC->hasFocus())
		lpC->setFocus();
}

#define KVI_TILE_METHOD_ANODINE 0
#define KVI_TILE_METHOD_PRAGMA4HOR 1
#define KVI_TILE_METHOD_PRAGMA4VER 2
#define KVI_TILE_METHOD_PRAGMA6HOR 3
#define KVI_TILE_METHOD_PRAGMA6VER 4
#define KVI_TILE_METHOD_PRAGMA9HOR 5
#define KVI_TILE_METHOD_PRAGMA9VER 6

#define KVI_NUM_TILE_METHODS 7

void KviMdiManager::fillWindowPopup()
{
	m_pWindowPopup->clear();

	m_pWindowPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_CASCADEWINDOWS)),(__tr2qs("&Cascade Windows")),this,SLOT(cascadeWindows()));
	m_pWindowPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_CASCADEWINDOWS)),(__tr2qs("Cascade &Maximized")),this,SLOT(cascadeMaximized()));

	m_pWindowPopup->insertSeparator();
	m_pWindowPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_TILEWINDOWS)),(__tr2qs("&Tile Windows")),this,SLOT(tile()));

	m_pTileMethodPopup->clear();
	int id = m_pTileMethodPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_AUTOTILEWINDOWS)),(__tr2qs("&Auto Tile")),this,SLOT(toggleAutoTile()));
	m_pTileMethodPopup->setItemChecked(id,KVI_OPTION_BOOL(KviOption_boolAutoTileWindows));
	m_pTileMethodPopup->setItemParameter(id,-1);
	m_pTileMethodPopup->insertSeparator();
	int ids[KVI_NUM_TILE_METHODS];
	ids[KVI_TILE_METHOD_ANODINE] = m_pTileMethodPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_TILEWINDOWS)),(__tr2qs("Anodine's Full Grid")));
	m_pTileMethodPopup->setItemParameter(ids[KVI_TILE_METHOD_ANODINE],KVI_TILE_METHOD_ANODINE);
	ids[KVI_TILE_METHOD_PRAGMA4HOR] = m_pTileMethodPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_TILEWINDOWS)),(__tr2qs("Pragma's Horizontal 4-Grid")));
	m_pTileMethodPopup->setItemParameter(ids[KVI_TILE_METHOD_PRAGMA4HOR],KVI_TILE_METHOD_PRAGMA4HOR);
	ids[KVI_TILE_METHOD_PRAGMA4VER] = m_pTileMethodPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_TILEWINDOWS)),(__tr2qs("Pragma's Vertical 4-Grid")));
	m_pTileMethodPopup->setItemParameter(ids[KVI_TILE_METHOD_PRAGMA4VER],KVI_TILE_METHOD_PRAGMA4VER);
	ids[KVI_TILE_METHOD_PRAGMA6HOR] = m_pTileMethodPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_TILEWINDOWS)),(__tr2qs("Pragma's Horizontal 6-Grid")));
	m_pTileMethodPopup->setItemParameter(ids[KVI_TILE_METHOD_PRAGMA6HOR],KVI_TILE_METHOD_PRAGMA6HOR);
	ids[KVI_TILE_METHOD_PRAGMA6VER] = m_pTileMethodPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_TILEWINDOWS)),(__tr2qs("Pragma's Vertical 6-Grid")));
	m_pTileMethodPopup->setItemParameter(ids[KVI_TILE_METHOD_PRAGMA6VER],KVI_TILE_METHOD_PRAGMA6VER);
	ids[KVI_TILE_METHOD_PRAGMA9HOR] = m_pTileMethodPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_TILEWINDOWS)),(__tr2qs("Pragma's Horizontal 9-Grid")));
	m_pTileMethodPopup->setItemParameter(ids[KVI_TILE_METHOD_PRAGMA9HOR],KVI_TILE_METHOD_PRAGMA9HOR);
	ids[KVI_TILE_METHOD_PRAGMA9VER] = m_pTileMethodPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_TILEWINDOWS)),(__tr2qs("Pragma's Vertical 9-Grid")));
	m_pTileMethodPopup->setItemParameter(ids[KVI_TILE_METHOD_PRAGMA9VER],KVI_TILE_METHOD_PRAGMA9VER);

	if(KVI_OPTION_UINT(KviOption_uintTileMethod) >= KVI_NUM_TILE_METHODS)KVI_OPTION_UINT(KviOption_uintTileMethod) = KVI_TILE_METHOD_PRAGMA9HOR;
	m_pTileMethodPopup->setItemChecked(ids[KVI_OPTION_UINT(KviOption_uintTileMethod)],true);

	m_pWindowPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_TILEWINDOWS)),(__tr2qs("Tile Met&hod")),m_pTileMethodPopup);

	m_pWindowPopup->insertSeparator();
	m_pWindowPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_MAXVERTICAL)),(__tr2qs("Expand &Vertically")),this,SLOT(expandVertical()));
	m_pWindowPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_MAXHORIZONTAL)),(__tr2qs("Expand &Horizontally")),this,SLOT(expandHorizontal()));

	m_pWindowPopup->insertSeparator();
	m_pWindowPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_MINIMIZE)),(__tr2qs("Mi&nimize All")),this,SLOT(minimizeAll()));
	m_pWindowPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_RESTORE)),(__tr2qs("&Restore all")),this,SLOT(restoreAll()));

	m_pWindowPopup->insertSeparator();
	int i = 100;
	QString szItem;
	QString szCaption;
	QList<QMdiSubWindow*> tmp = subWindowList(QMdiArea::StackingOrder);
	QListIterator<QMdiSubWindow*> it(tmp);

	KviMdiChild * lpC;

	while (it.hasNext())
	{
		lpC = (KviMdiChild *) it.next();

		if (!lpC->inherits("KviMdiChild"))
		{
			i++;
			continue;
		}

		szItem.setNum(((uint)i)-99);
		szItem+=". ";

		szCaption = lpC->windowTitle();
		if(szCaption.length() > 30)
		{
			QString trail = szCaption.right(12);
			szCaption.truncate(12);
			szCaption+="...";
			szCaption+=trail;
		}

		if(lpC->state()==KviMdiChild::Minimized)
		{
			szItem+="(";
			szItem+=szCaption;
			szItem+=")";
		} else szItem+=szCaption;

		const QPixmap * pix = lpC->icon();

		if (pix && !(pix->isNull()))
		{
			m_pWindowPopup->insertItem(*pix, szItem,i);
		} else {
			m_pWindowPopup->insertItem(szItem);
		}

		//this is useless, since the windows are listed in stacking order, the active one
		//will always be the last anyway.
		//if(lpC==currentSubWindow())
		//	m_pWindowPopup->setItemChecked(i, true );
		i++;
	}
}

void KviMdiManager::menuActivated(int id)
{
	if(id<100)return;
	id-=100;
	QList<QMdiSubWindow *> tmp = subWindowList(QMdiArea::StackingOrder);

	if(id >= tmp.count()) return;
	if (!tmp.at(id)->inherits("KviMdiChild")) return;

	KviMdiChild * lpC = (KviMdiChild *) tmp.at(id);

	if(!lpC) return;
	if(lpC->state() == KviMdiChild::Minimized) lpC->restore();

	setTopChild(lpC);
}

void KviMdiManager::ensureNoMaximized()
{
	QList<QMdiSubWindow *> tmp = subWindowList(QMdiArea::StackingOrder);

	KviMdiChild * lpC;

	for(int i = 0; i < tmp.count(); i++)
	{
		if (tmp.at(i)->inherits("KviMdiChild"))
		{
			lpC = (KviMdiChild *) tmp.at(i);
			if(lpC->state() == KviMdiChild::Maximized)lpC->restore();
		}
	}
}

void KviMdiManager::tileMethodMenuActivated(int id)
{
	int idx = m_pTileMethodPopup->itemParameter(id);

	if(idx < 0) idx = 0;
	if(idx >= KVI_NUM_TILE_METHODS) idx = KVI_TILE_METHOD_PRAGMA9VER;

	KVI_OPTION_UINT(KviOption_uintTileMethod) = idx;

	//we don't check the m_bInSDIMode value here, since it was
	//the user forcing windows to be tiled

	if(KVI_OPTION_BOOL(KviOption_boolAutoTileWindows)) tile();
}

void KviMdiManager::cascadeWindows()
{
	cascadeSubWindows();
}

void KviMdiManager::cascadeMaximized()
{
	cascadeSubWindows();
	QList<QMdiSubWindow *> tmp = subWindowList();
	KviMdiChild * lpC;

	for(int i = 0; i < tmp.count(); i++)
	{
		if (tmp.at(i)->inherits("KviMdiChild"))
		{
			lpC = (KviMdiChild *) tmp.at(i);
			if(lpC->state() != KviMdiChild::Minimized)
			{
				QPoint pnt(lpC->pos());
				QSize curSize(viewport()->width() - pnt.x(),viewport()->height() - pnt.y());
				if((lpC->minimumSize().width() > curSize.width()) ||
					(lpC->minimumSize().height() > curSize.height()))lpC->resize(lpC->minimumSize());
				else lpC->resize(curSize);
			}
		}
	}
}

void KviMdiManager::expandVertical()
{
	ensureNoMaximized();
	QList<QMdiSubWindow *> tmp = subWindowList(QMdiArea::StackingOrder);
	KviMdiChild * lpC;

	for(int i = 0; i < tmp.count(); i++)
	{
		if (tmp.at(i)->inherits("KviMdiChild"))
		{
			lpC = (KviMdiChild *) tmp.at(i);
			if(lpC->state() != KviMdiChild::Minimized)
			{
				lpC->move(lpC->x(),0);
				lpC->resize(lpC->width(),viewport()->height());
			}
		}
	}
}

void KviMdiManager::expandHorizontal()
{
	ensureNoMaximized();
	QList<QMdiSubWindow *> tmp = subWindowList(QMdiArea::StackingOrder);
	KviMdiChild * lpC;

	for(int i = 0; i < tmp.count(); i++)
	{
		if (tmp.at(i)->inherits("KviMdiChild"))
		{
			lpC = (KviMdiChild *) tmp.at(i);
			if(lpC->state() != KviMdiChild::Minimized)
			{
				lpC->move(0,lpC->y());
				lpC->resize(viewport()->width(),lpC->height());
			}
		}
	}
}

void KviMdiManager::minimizeAll()
{
	QList<QMdiSubWindow *> tmp = subWindowList(QMdiArea::StackingOrder);

	KviMdiChild * lpC;

	for(int i = 0; i < tmp.count(); i++)
	{
		if (tmp.at(i)->inherits("KviMdiChild"))
		{
			lpC = (KviMdiChild *) tmp.at(i);
			if(lpC->state() != KviMdiChild::Minimized) lpC->minimize();
		}
	}
}


void KviMdiManager::restoreAll()
{
	QList<QMdiSubWindow *> tmp = subWindowList(QMdiArea::StackingOrder);

	KviMdiChild * lpC;

	for(int i = 0; i < tmp.count(); i++)
	{
		if (tmp.at(i)->inherits("KviMdiChild"))
		{
			lpC = (KviMdiChild *) tmp.at(i);
			if(lpC->state() == KviMdiChild::Minimized) lpC->restore();
		}
	}
	if(!m_bInSDIMode)
		if(KVI_OPTION_BOOL(KviOption_boolAutoTileWindows))tile();
}

int KviMdiManager::getVisibleChildCount()
{
	QList<QMdiSubWindow *> l = subWindowList();

	int cnt = 0;
	int i = 0;
	for(i = 0; i < l.count(); i++)
	{
		if(!l.at(i)->isMinimized()) cnt++;
	}
	return cnt;
}

void KviMdiManager::tile()
{
	switch(KVI_OPTION_UINT(KviOption_uintTileMethod))
	{
		case KVI_TILE_METHOD_ANODINE:      tileAnodine(); break;
		case KVI_TILE_METHOD_PRAGMA4HOR:   tileAllInternal(4,true); break;
		case KVI_TILE_METHOD_PRAGMA4VER:   tileAllInternal(4,false); break;
		case KVI_TILE_METHOD_PRAGMA6HOR:   tileAllInternal(6,true); break;
		case KVI_TILE_METHOD_PRAGMA6VER:   tileAllInternal(6,false); break;
		case KVI_TILE_METHOD_PRAGMA9HOR:   tileAllInternal(9,true); break;
		case KVI_TILE_METHOD_PRAGMA9VER:   tileAllInternal(9,false); break;
		default:
			KVI_OPTION_UINT(KviOption_uintTileMethod) = KVI_TILE_METHOD_PRAGMA9HOR;
			tileAllInternal(9,true);
		break;
	}
}

void KviMdiManager::toggleAutoTile()
{
	if(KVI_OPTION_BOOL(KviOption_boolAutoTileWindows))
	{
		KVI_OPTION_BOOL(KviOption_boolAutoTileWindows) = false;
	} else {
		KVI_OPTION_BOOL(KviOption_boolAutoTileWindows) = true;
		tile();
	}
}

void KviMdiManager::tileAllInternal(int maxWnds, bool bHorizontal) //int maxWnds,bool bHorizontal
{

	//NUM WINDOWS =           1,2,3,4,5,6,7,8,9
	static int colstable[9]={ 1,1,1,2,2,2,3,3,3 }; //num columns
	static int rowstable[9]={ 1,2,3,2,3,3,3,3,3 }; //num rows
	static int lastwindw[9]={ 1,1,1,1,2,1,3,2,1 }; //last window multiplier
	static int colrecall[9]={ 0,0,0,3,3,3,6,6,6 }; //adjust self
	static int rowrecall[9]={ 0,0,0,0,4,4,4,4,4 }; //adjust self

	int * pColstable = bHorizontal ? colstable : rowstable;
	int * pRowstable = bHorizontal ? rowstable : colstable;
	int * pColrecall = bHorizontal ? colrecall : rowrecall;
	int * pRowrecall = bHorizontal ? rowrecall : colrecall;

	ensureNoMaximized();
	if (g_pApp->kviClosingDown()) return;

	KviMdiChild * lpTop = topChild();
	if (!lpTop) return;

	int numVisible = getVisibleChildCount();

	if (numVisible < 1) return;

	int numToHandle = ((numVisible > maxWnds) ? maxWnds : numVisible);
	int xQuantum = viewport()->width() / pColstable[numToHandle-1];

	if(xQuantum < ((lpTop->minimumSize().width() > KVI_MDICHILD_MIN_WIDTH) ? lpTop->minimumSize().width() : KVI_MDICHILD_MIN_WIDTH))
	{
		if (pColrecall[numToHandle-1] == 0) qDebug("Tile : Not enouh space");
			else tileAllInternal(pColrecall[numToHandle-1], bHorizontal);
		return;
	}

	int yQuantum = viewport()->height() / pRowstable[numToHandle-1];

	if(yQuantum < ((lpTop->minimumSize().height() > KVI_MDICHILD_MIN_HEIGHT) ? lpTop->minimumSize().height() : KVI_MDICHILD_MIN_HEIGHT))
	{
		if (pRowrecall[numToHandle-1] == 0) qDebug("Tile : Not enough space");
			else tileAllInternal(pRowrecall[numToHandle-1], bHorizontal);
		return;
	}

	int curX = 0;
	int curY = 0;
	int curRow = 1;
	int curCol = 1;
	int curWin = 1;

	QList<QMdiSubWindow *> tmp = subWindowList(QMdiArea::StackingOrder);

	for(int i = 0; i < tmp.count(); i++)
	{
		KviMdiChild * lpC = (KviMdiChild*) tmp.at(i);

		if(lpC->state()!=KviMdiChild::Minimized)
		{
			if((curWin%numToHandle)==0)
			{
				lpC->move(curX, curY);
				lpC->resize(xQuantum * lastwindw[numToHandle-1], yQuantum);
			} else {
				lpC->move(curX, curY);
				lpC->resize(xQuantum, yQuantum);
			}
			//example : 12 windows : 3 cols 3 rows
			if (curCol < pColstable[numToHandle-1])
			{ //curCol < 3
				curX += xQuantum; //add a column in the same row
				curCol++;       //increase current column
			} else {
				curX = 0;         //new row
				curCol = 1;       //column 1
				if (curRow < pRowstable[numToHandle-1])
				{ //curRow < 3
					curY += yQuantum; //add a row
					curRow++;
				} else {
					curY = 0;         //restart from beginning
					curRow = 1;
				}
			}
			curWin++;
		}
	}
	if(lpTop)lpTop->setFocus();
}

void KviMdiManager::tileAnodine()
{
	this->tileSubWindows();
}

void KviMdiManager::slotSubWindowActivated(QMdiSubWindow * pMdiChild)
{
	if(pMdiChild)
	{
		if(((KviMdiChild*)pMdiChild)->client() && g_pFrame->isActiveWindow())
		{
			//qDebug("subwindowactivated %p %s",pMdiChild, ((KviMdiChild*)pMdiChild)->plainCaption().toUtf8().data());
			g_pFrame->childWindowActivated((KviWindow *)((KviMdiChild*)pMdiChild)->client());
		} else {
			//qDebug("(inactive) subwindowactivated %p %s",pMdiChild, ((KviMdiChild*)pMdiChild)->plainCaption().toUtf8().data());
		}
	} else {
		//last subwindow deactivated
		//qDebug("subwindowactivated 0x0");
	}
}

bool KviMdiManager::eventFilter(QObject *obj, QEvent *event)
{
	if(
		(
			(event->type() == QEvent::KeyPress) ||
			(event->type() == QEvent::KeyRelease)
		) &&
		m_bInSDIMode
	)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		if (	(keyEvent->modifiers() & Qt::MetaModifier) ||
			(keyEvent->modifiers() & Qt::ControlModifier)
		)
		{
			// While in sdi mode, avoid qt4's internal window switching
			if(keyEvent->key() == Qt::Key_Tab)
			{
				if(event->type() == QEvent::KeyRelease) g_pFrame->switchToNextWindow();
				return true;
			}
			if(keyEvent->key() == Qt::Key_Backtab)
			{
				if(event->type() == QEvent::KeyRelease) g_pFrame->switchToPrevWindow();
				return true;
			}
		}
	} else {
		/*
		 * Filter out some (de)activation events from the QMdiArea handling
	         * this fixed #425 and #519 (unwanted mdichild activation when KviFrame is not the active window)
		 */
		if(	event->type() == QEvent::ApplicationActivate ||
			event->type() == QEvent::ApplicationDeactivate
		) return true;
	}
	return QMdiArea::eventFilter(obj, event);
}

