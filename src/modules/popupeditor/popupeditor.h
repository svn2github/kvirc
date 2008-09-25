#ifndef _POPUPEDITOR_H_
#define _POPUPEDITOR_H_
//=============================================================================
//
//   File : popupeditor.h
//   Creation date : Mon Dec 23 2002 20:24:55 CEST by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2002-2008 Szymon Stefanek (pragma at kvirc dot net)
//
//   This program is FREE software. You can redistribute it and/or
//   modify it under the linkss of the GNU General Public License
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

#include "kvi_window.h"
#include "kvi_string.h"
#include "kvi_tal_treewidget.h"
#include "kvi_tal_popupmenu.h"

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>

class KviKvsPopupMenu;
class KviKvsPopupMenuItem;
class KviScriptEditor;
class KviMenuTreeWidgetItem;

// KviPopupTreeWidgetItem
class KviPopupTreeWidgetItem : public KviTalTreeWidgetItem
{
public:
	enum Type { Item , Menu , Separator , Label , Epilogue , Prologue , ExtMenu };
public:
	KviPopupTreeWidgetItem(KviTalTreeWidget * pTreeWidget,KviPopupTreeWidgetItem * after,Type t);
	KviPopupTreeWidgetItem(KviPopupTreeWidgetItem * parent,KviPopupTreeWidgetItem * after,Type t);
public:
	Type                   m_type;
	QString                m_szText;
	QString                m_szCondition;
	QString                m_szIcon;
	QString                m_szCode;
	QString                m_szId;
private:
	void init();
public:
	void setItemText(const QString & szText);
	void setCondition(const QString & szCondition);
	void setIcon(const QString & szIcon);
	void setCode(const QString & szCode);
	void setId(const QString & szId);
};


class KviSinglePopupEditor : public QWidget
{
	Q_OBJECT
public:
	KviSinglePopupEditor(QWidget * par);
	~KviSinglePopupEditor();
protected:
	QPushButton          * m_pMenuButton;
	KviKvsPopupMenu      * m_pClipboard;
	KviKvsPopupMenu      * m_pTestPopup;
	KviPopupTreeWidgetItem * m_pLastSelectedItem;
	KviTalTreeWidget     * m_pTreeWidget;
	QLineEdit            * m_pNameEditor;
	KviScriptEditor      * m_pEditor;
	QLineEdit            * m_pTextEditor;
	QLineEdit            * m_pIdEditor;
	QLineEdit            * m_pIconEditor;
	QLineEdit            * m_pConditionEditor;
	QLineEdit            * m_pExtNameEditor;
	KviTalPopupMenu      * m_pContextPopup;
public:
	void edit(KviMenuTreeWidgetItem * it);
	KviKvsPopupMenu * getMenu();
protected:
	// theItem is the item above the first item that has to be inserted
	void populateMenu(KviKvsPopupMenu * pop,KviPopupTreeWidgetItem * par,KviPopupTreeWidgetItem * theItem = 0);
	void saveLastSelectedItem();
	void addItemToMenu(KviKvsPopupMenu * pop,KviPopupTreeWidgetItem * par);
	KviPopupTreeWidgetItem * newItem(KviPopupTreeWidgetItem * par,KviPopupTreeWidgetItem * after,KviPopupTreeWidgetItem::Type t);
	KviPopupTreeWidgetItem * newItemBelow(KviPopupTreeWidgetItem * it,KviPopupTreeWidgetItem::Type t);
	KviPopupTreeWidgetItem * newItemAbove(KviPopupTreeWidgetItem * it,KviPopupTreeWidgetItem::Type t);
	KviPopupTreeWidgetItem * newItemInside(KviPopupTreeWidgetItem * it,KviPopupTreeWidgetItem::Type t);
	void createNewItemAboveLastSelected(KviPopupTreeWidgetItem::Type t);
	void createNewItemBelowLastSelected(KviPopupTreeWidgetItem::Type t);
	void createNewItemInsideLastSelected(KviPopupTreeWidgetItem::Type t);
	KviPopupTreeWidgetItem * findMatchingItem(KviKvsPopupMenuItem * it,KviPopupTreeWidgetItem * item);
protected slots:
	void contextCut();
	void contextCopy();
	void contextPasteBelow();
	void contextPasteAbove();
	void contextPasteInside();
	void contextNewSeparatorBelow();
	void contextNewSeparatorAbove();
	void contextNewSeparatorInside();
	void contextNewItemBelow();
	void contextNewItemAbove();
	void contextNewItemInside();
	void contextNewMenuBelow();
	void contextNewMenuAbove();
	void contextNewMenuInside();
	void contextNewExtMenuBelow();
	void contextNewExtMenuAbove();
	void contextNewExtMenuInside();
	void contextNewLabelBelow();
	void contextNewLabelAbove();
	void contextNewLabelInside();
	void contextNewPrologue();
	void contextNewEpilogue();
	void selectionChanged();
	void itemPressed(KviTalTreeWidgetItem * item, int column);
	void testPopup();
	void testModeMenuItemClicked(KviKvsPopupMenuItem * it);
};



class KviMenuTreeWidgetItem : public KviTalTreeWidgetItem
{
public:
	KviMenuTreeWidgetItem(KviTalTreeWidget * par,KviKvsPopupMenu * popup);
	~KviMenuTreeWidgetItem();
public:
	KviKvsPopupMenu * m_pPopup;
public:
	KviKvsPopupMenu * popup(){ return m_pPopup; };
public:
	void replacePopup(KviKvsPopupMenu * popup);
};


class KviPopupEditor : public QWidget
{
	Q_OBJECT
public:
	KviPopupEditor(QWidget * par);
	~KviPopupEditor();
public:
	KviSinglePopupEditor * m_pEditor;
	KviTalTreeWidget     * m_pTreeWidget;
	KviMenuTreeWidgetItem  * m_pLastEditedItem;
	bool                   m_bOneTimeSetupDone;
	KviTalPopupMenu      * m_pContextPopup;
public:
	void commit();
	void exportPopups(bool);
protected slots:
	void currentItemChanged(KviTalTreeWidgetItem *it,KviTalTreeWidgetItem *prev);
	void newPopup();
	void exportAll();
	void exportSelected();
	void exportCurrentPopup();
	void removeCurrentPopup();
	void itemPressed(KviTalTreeWidgetItem * item, int column);
protected:
	void showEvent(QShowEvent *e);
	void getExportPopupBuffer(QString &buffer,KviMenuTreeWidgetItem * it);
private:
	void oneTimeSetup();
	void saveLastEditedItem();
	void getUniquePopupName(KviMenuTreeWidgetItem * it,QString &buffer);
};



class KviPopupEditorWindow : public KviWindow
{
	Q_OBJECT
public:
	KviPopupEditorWindow(KviFrame * lpFrm);
	~KviPopupEditorWindow();
protected:
	KviPopupEditor * m_pEditor;
	QWidget        * m_pBase;
protected:
	virtual QPixmap * myIconPtr();
	virtual void fillCaptionBuffers();
	virtual void resizeEvent(QResizeEvent *e);
	virtual void getConfigGroupName(KviStr &szName);
	virtual void saveProperties(KviConfig *);
	virtual void loadProperties(KviConfig *);
protected slots:
	void cancelClicked();
	void okClicked();
	void applyClicked();
};

#endif //_POPUPEDITOR_H_
