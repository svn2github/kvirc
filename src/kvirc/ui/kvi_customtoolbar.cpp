//=============================================================================
//
//   File : kvi_customtoolbar.cpp
//   Creation date : Sun 21 Nov 2004 05:28:57 by Szymon Stefanek
//
//   This file is part of the KVIrc IRC Client distribution
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
//   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
//=============================================================================



#include "kvi_customtoolbar.h"
#include "kvi_frame.h"
#include "kvi_locale.h"
#include "kvi_app.h"
#include "kvi_actionmanager.h"
#include "kvi_customtoolbardescriptor.h"
#include "kvi_tal_popupmenu.h"

#include <QCursor>
#include <QLayout>
#include <QPixmap>
#include <QPainter>
#include <QStyle>
#include <QEvent>
#include <QStyleOption>
#include <QDragEnterEvent>
#include <QMimeData>

KviCustomToolBar::KviCustomToolBar(KviCustomToolBarDescriptor * pDesc, const QString & szLabel, Qt::ToolBarArea type, bool bNewLine, const char * pcName)
: KviToolBar(szLabel,type,bNewLine,pcName)
{
	m_pDescriptor = pDesc;
	m_pMovedChild = 0;
	m_pMovedAction = 0;
	m_pDraggedChild = 0;
	m_pFilteredChildren = 0;
	setAcceptDrops(true);
	connect(KviActionManager::instance(),SIGNAL(beginCustomizeToolBars()),this,SLOT(beginCustomize()));
	connect(KviActionManager::instance(),SIGNAL(endCustomizeToolBars()),this,SLOT(endCustomize()));
// 	setMinimumSize(20,20);
	pDesc->registerToolBar(this);
	if(KviActionManager::customizingToolBars())
		beginCustomize(); // because we will not get the signal
}

KviCustomToolBar::~KviCustomToolBar()
{
	if(KviActionManager::customizingToolBars())
		syncDescriptor(); // because we will not get endCustomize()
	m_pDescriptor->unregisterToolBar(this);

	if(KviActionManager::customizingToolBars())
	{
		if(KviActionManager::currentToolBar() == this)
			KviActionManager::instance()->setCurrentToolBar(0);
	}

	if(m_pFilteredChildren) 
		delete m_pFilteredChildren;
}

void KviCustomToolBar::paintEvent(QPaintEvent * e)
{
	KviToolBar::paintEvent(e);

	if(KviActionManager::customizingToolBars() && (KviActionManager::currentToolBar() == this))
	{
		QPainter *p = new QPainter(this);
		QPalette pal=palette();
		QColor col=pal.highlight().color();
		col.setAlpha(127);
		p->fillRect(rect(),QBrush(col));
		delete p;
	}
}

void KviCustomToolBar::filteredChildDestroyed()
{
	if(!m_pFilteredChildren)
		return;
	const QObject * o = sender();
	m_pFilteredChildren->remove((void *)o);
}

void KviCustomToolBar::filterChild(QObject * o)
{
	bool * pBool = new bool(((QWidget *)o)->isEnabled());
	if(m_pFilteredChildren)
		m_pFilteredChildren->insert(o,pBool);
	if(!*pBool)
		((QWidget *)o)->setEnabled(true);
	o->installEventFilter(this);
	connect(o,SIGNAL(destroyed()),this,SLOT(filteredChildDestroyed()));
}

void KviCustomToolBar::unfilterChild(QObject * o)
{
	if(m_pFilteredChildren)
	{
		bool * pBool = m_pFilteredChildren->find(o);
		if(pBool)
		{
			if(!*pBool)
				((QWidget *)o)->setEnabled(false);
			o->removeEventFilter(this);
			disconnect(o,SIGNAL(destroyed()),this,SLOT(filteredChildDestroyed()));
		}
	}
}

void KviCustomToolBar::beginCustomize()
{
	if(m_pFilteredChildren)
		delete m_pFilteredChildren;

	m_pFilteredChildren = new KviPointerHashTable<void *,bool>;
	m_pFilteredChildren->setAutoDelete(true);
	// filter the events for all the children
	QList<QObject *> list = children();
	for(QList<QObject *>::Iterator it = list.begin(); it != list.end(); ++it)
	{
		if((*it)->isWidgetType())
			filterChild(*it);
	}
}

void KviCustomToolBar::endCustomize()
{
	// stop filtering events
	QList<QObject *> list = children();
	for(QList<QObject *>::Iterator it = list.begin(); it != list.end(); ++it)
	{
		if((*it)->isWidgetType())
			unfilterChild(*it);
	}

	// FIXME: We SHOULD MAKE SURE that the children are re-enabled...
	// this could be done by calling setEnabled(isEnabled()) on each action ?
	if(m_pFilteredChildren)
	{
		delete m_pFilteredChildren;
		m_pFilteredChildren = 0;
	}
	syncDescriptor();
}

void KviCustomToolBar::syncDescriptor()
{
	// store the item order in the descriptor
	// There was boxLayouts
	QLayout * pLayout = layout();
	m_pDescriptor->actions()->clear();
	for(int i=0; i < pLayout->count(); i++)
	{
		if(QWidget * w = pLayout->itemAt(i)->widget())
		{
#ifdef COMPILE_ON_MAC
			// on mac, we are not able to delete unused widgets from the toolbar while dragging;
			// instead, we only hide them. now it's time to clean them'up. (ticket #803)
			if(w->isVisible())
			{
				m_pDescriptor->actions()->append(new QString(w->objectName()));
			} else {
				w->deleteLater();
			}
#else
			m_pDescriptor->actions()->append(new QString(w->objectName()));
#endif
		}
	}
}

void KviCustomToolBar::childEvent(QChildEvent * e)
{
	if(KviActionManager::customizingToolBars())
	{
		// this is useful for droppped and dragged-out children
		if(e->type() == QEvent::ChildAdded)
		{
			if(e->child()->isWidgetType())
				filterChild(e->child());
			goto done;
		}

		if(e->type() == QEvent::ChildRemoved)
		{
			if(e->child()->isWidgetType())
				unfilterChild(e->child());
			goto done;
		}
	}
done:
	KviToolBar::childEvent(e);
}
 
void KviCustomToolBar::dragEnterEvent(QDragEnterEvent * e)
{
	if(!KviActionManager::customizingToolBars())
		return;

	KviActionManager::instance()->setCurrentToolBar(this);
	QString szText;

	if(e->mimeData()->hasText())
	{
		szText=e->mimeData()->text();
		if(!szText.isEmpty())
		{
			KviAction * pAction = KviActionManager::instance()->getAction(szText);
			if(pAction)
			{
				m_pDraggedChild = pAction->addToCustomToolBar(this);
				e->acceptProposedAction();
				QEvent ev(QEvent::LayoutRequest);
				QApplication::sendEvent(this,&ev);
			} else e->ignore();
		} else e->ignore();
	} else e->ignore();
}

void KviCustomToolBar::dragMoveEvent(QDragMoveEvent *e)
{
	if(!m_pDraggedChild)
		return;

	int iMe = -1;
	int iIdx = dropIndexAt(e->pos(),m_pDraggedChild,&iMe);
// 	debug("DROP INDEX IS %d, ME IS %d",iIdx,iMe);
	if(iIdx == iMe) return; // would move over itself

	QWidget * pWidgetToMove = widgetAt(iIdx > iMe ? iIdx+1 : iIdx);
// 	debug("SEARCHING FOR WIDGET TO MOVE AT %d AND FOUND %x (ME=%x)",iIdx > iMe ? iIdx+1 : iIdx,pWidgetToMove,pChild);
	if(pWidgetToMove == m_pDraggedChild)
		return; // hmmm

	bool bDone = false;
	QAction * pMyOwnAction = actionForWidget(m_pDraggedChild);
	if(!pMyOwnAction)
		return;

	QAction * pAction=0;
	removeAction(pMyOwnAction);
	if(pWidgetToMove)
	{
		pAction = actionForWidget(pWidgetToMove);
		if(pAction)
		{
// 			debug("AND GOT ACTION FOR THAT WIDGET\n");

			bDone = true;
			pAction = insertWidget(pAction,m_pDraggedChild);
		}
	} else {
		pAction = addWidget(m_pDraggedChild);
	}

	if(!bDone)
		pAction = addWidget(m_pDraggedChild);
	pAction->setVisible(true);

	QEvent ev(QEvent::LayoutRequest);
	QApplication::sendEvent(this,&ev);
}

void KviCustomToolBar::dragLeaveEvent(QDragLeaveEvent *)
{
	if(m_pDraggedChild)
	{
		if(m_pFilteredChildren)
			m_pFilteredChildren->remove(m_pDraggedChild); // just to be sure
		//m_pDraggedChild->reparent(0);
		QAction* widgetAction = actionForWidget(m_pDraggedChild);
		if(widgetAction)
			widgetAction->setVisible(false);
		//ifdef workaround for #803
#ifndef COMPILE_ON_MAC
		m_pDraggedChild->deleteLater(); // don't delete it now: it's going to crash with recent Qt versions
#endif
		//delete m_pDraggedChild;
		m_pDraggedChild = 0;
	}
}

void KviCustomToolBar::dropEvent(QDropEvent * e)
{
	if(!m_pDraggedChild)
		return;
	m_pDraggedChild = 0;

	if(e->mimeData()->hasText())
		e->acceptProposedAction();
	// nuthin :)
}

int KviCustomToolBar::dropIndexAt(const QPoint & pnt, QWidget * pExclude, int * piExcludeIdx)
{
	// find the widget at the current poisition
	// treating exclude as if it was going to be removed
	// find also the exclude index if needed
	QLayout * pLayout = layout();
	if(piExcludeIdx)
		*piExcludeIdx = -1;
	int iIdx = 0;

	if(!pLayout)
		return 0;
//	QLayoutIterator it = l->iterator();

	// find the children with minimum distance
	int iMinDistIdx = -1;
	QWidget * pMinDistW = 0;
	unsigned int uMinDist = 0xffffffff;
	int iExcludeIdx = -1;
	QPoint pntExclude;
	QWidget * pWidget = 0;

	for (int i=0; i < pLayout->count(); i++)
	{
		if((pWidget = pLayout->itemAt(i)->widget()))
		{
			if(uMinDist != 0)
			{
				int iRight = pWidget->x() + pWidget->width();
				int iBottom = pWidget->y() + pWidget->height();
				if((pnt.x() >= pWidget->x()) && (pnt.y() >= pWidget->y()) && (pnt.x() <= iRight) && (pnt.y() <= iBottom))
				{
					// inside the widget
					// distance 0
					pMinDistW = pWidget;
					iMinDistIdx = iIdx;
					uMinDist = 0;
				} else {
					// outside the widget
					// compute the distance
					unsigned int uXDist = (pnt.x() < pWidget->x()) ? (pWidget->x() - pnt.x()) : (pnt.x() > iRight ? (pnt.x() - iRight) : 0);
					unsigned int uYDist = (pnt.y() < pWidget->y()) ? (pWidget->y() - pnt.y()) : (pnt.y() > iBottom ? (pnt.y() - iBottom) : 0);
					if((uXDist < 8192) && (uYDist < 8192))
					{
						// it is in reasonable rect
						unsigned int uDist = (uXDist * uXDist) + (uYDist * uYDist);
						if(uDist < uMinDist)
						{
							// min distance for now
							uMinDist = uDist;
							pMinDistW = pWidget;
							iMinDistIdx = iIdx;
						}
					} // else the user has a really HUUUGE screen
				}
			} // else the minimum distance widget has already been found
			if(pWidget == pExclude)
			{
				iExcludeIdx = iIdx;
				pntExclude = pWidget->pos();
			}
		}
		iIdx++;
	}

	if(!pMinDistW)
	{
		// ops.. not found at all (empty toolbar or really far from any button)
		if(orientation() == Qt::Horizontal)
		{
			if(pnt.x() < (width() / 2))
				iMinDistIdx = 0; // insert at position 0
			else iMinDistIdx = iIdx;
			// else insert at the last position found
		} else {
			if(pnt.y() < (height() / 2))
				iMinDistIdx = 0; // insert at position 0
			else iMinDistIdx = iIdx;
			// else insert at the last position found
		}
	} else {
		// got it, check for the exclude idx
		if((iExcludeIdx == -1) || (iExcludeIdx != iMinDistIdx))
		{
			// would not put it over exclude idx
			// check if we have to stay on right or left of the widget found
			if(orientation() == Qt::Horizontal)
			{
				if(pnt.x() > (pMinDistW->x() + (pMinDistW->width() / 2)))
					iMinDistIdx++; // need to put it on the right
			} else {
				if(pnt.y() > (pMinDistW->y() + (pMinDistW->height() / 2)))
					iMinDistIdx++; // need to put it below
			}
		}

		// ok , check again (we might have moved exactly over exclude idx now!)
		if((iExcludeIdx != -1) && (iExcludeIdx != iMinDistIdx))
		{
			// got the exclude idx by the way and wouldn't put exactly over it
			// check if exclude idx is "before" the current possible insert position
			// if it is , then lower down the index by one
			if(orientation() == Qt::Horizontal)
			{
				if(pnt.x() > pntExclude.x())
					iMinDistIdx--; // removing exclude will move everything one step back
			} else {
				if(pnt.y() > pntExclude.y())
					iMinDistIdx--; // removing exclude will move everything one step back
			}
		}
	}

	if(iMinDistIdx < 0)
		iMinDistIdx = 0;
	if(piExcludeIdx)
		*piExcludeIdx = iExcludeIdx;

	return iMinDistIdx;
}

QWidget * KviCustomToolBar::widgetAt(int iIndex)
{
	QLayout * pLayout = layout();
	if(!pLayout)
		return NULL;

	QLayoutItem * it = pLayout->itemAt(iIndex);
	if(!it)
		return NULL;

	return it->widget();
}

QAction * KviCustomToolBar::actionForWidget(QWidget * pWidget)
{
	return actionAt(pWidget->x() + 1,pWidget->y() + 1);
}

void KviCustomToolBar::mousePressEvent(QMouseEvent * e)
{
	if(KviActionManager::customizingToolBars())
		KviActionManager::instance()->setCurrentToolBar(this);
	KviToolBar::mousePressEvent(e);
}

bool KviCustomToolBar::eventFilter(QObject * o,QEvent * e)
{
	if(!KviActionManager::customizingToolBars())
		return KviToolBar::eventFilter(o,e); // anything here is done when customizing only
	if(e->type() == QEvent::Enter)
	{
		if(m_pMovedChild)
			return true; // kill it while moving other children
	}

	if(e->type() == QEvent::Leave)
	{
		if(m_pMovedChild)
			return true; // kill it while moving other children
	}

	if(e->type() == QEvent::MouseButtonPress)
	{
		//qDebug("mouse button pressed");
		KviActionManager::instance()->setCurrentToolBar(this);
		QMouseEvent * pEvent = (QMouseEvent *)e;
		if(pEvent->button() & Qt::LeftButton)
		{
			if(o->isWidgetType())
			{
				if(!(
					o->inherits("KviTalPopupMenu") ||
					o->inherits("QToolBarHandle") ||
					o->inherits("QDockWindowHandle") ||
					o->inherits("QDockWindowResizeHandle") ||
					o->inherits("QToolBarExtensionWidget")
					))
				{
					m_pMovedChild = (QWidget *)o;
					// allow resizing of children
					// FIXME: do it only if the child is really resizable
					if(m_pMovedChild->width() > 20) // might be an applet
					{
						if(pEvent->pos().x() > (m_pMovedChild->width() - 4))
						{
							m_pMovedChild = 0;
							return KviToolBar::eventFilter(o,e); // let the applet handle the event it
						}
					}
					m_pMovedAction=actionForWidget(m_pMovedChild);

					// drag out!
					QDrag * pDrag = new QDrag(this);
					QMimeData * pMime = new QMimeData();
					pMime->setText(m_pMovedChild->objectName());
					pDrag->setMimeData(pMime);
					KviAction * act = KviActionManager::instance()->getAction(m_pMovedChild->objectName());
					if(act)
					{
						QPixmap * pixie = act->bigIcon();
						if(pixie)
						{
							pDrag->setPixmap(*pixie);
							pDrag->setHotSpot(QPoint(3,3));
						}
					}

					m_pMovedAction->setVisible(false);

					QEvent ev(QEvent::LayoutRequest);
					QApplication::sendEvent(this,&ev);
					if(pDrag->exec(Qt::MoveAction) == Qt::MoveAction)
					{
						removeAction(m_pMovedAction);
					} else {
						// the user has probably failed to remove the action from the toolbar
						// flash the trashcan in the customize toolbars dialog
						KviActionManager::instance()->emitRemoveActionsHintRequest();
						m_pMovedAction->setVisible(true);

						QEvent ev(QEvent::LayoutRequest);
						QApplication::sendEvent(this,&ev);
					}
					m_pMovedChild=0;
					m_pMovedAction=0;
					return true;
				}
			}
		}
	}
	return KviToolBar::eventFilter(o,e);
}

KviCustomToolBarSeparator::KviCustomToolBarSeparator(KviCustomToolBar * pParent, const char * pcName)
: QWidget(pParent)
{
	setObjectName(pcName);
	m_pToolBar = pParent;
	setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
}

QSize KviCustomToolBarSeparator::sizeHint() const
{
	QStyleOption opt;
	opt.initFrom(this);
	int iExtent = style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent,&opt,this);

	if(m_pToolBar->orientation() == Qt::Horizontal)
		return QSize(iExtent,0);
	else return QSize(0,iExtent);
}

void KviCustomToolBarSeparator::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	QStyleOption opt;
	opt.initFrom(this);
	style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator,&opt,&p,this);
}
