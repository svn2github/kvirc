#ifndef _SHAREDFILESWINDOW_H_
#define _SHAREDFILESWINDOW_H_
//==================================================================================
//
//   File : sharedfileswindow.h
//   Creation date : Mon Apr 21 2003 23:14:12 CEST by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2003 Szymon Stefanek (pragma at kvirc dot net)
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
//==================================================================================

#include "kvi_window.h"
#include "kvi_string.h"
#include "kvi_sparser.h"
#include "kvi_console.h"
#include "kvi_moduleextension.h"
#include "kvi_sharedfiles.h"
#include "kvi_styled_controls.h"

#include "kvi_tal_listview.h"
#include <qdialog.h>

class QGridLayout; 
class QDateTimeEdit;
class QLabel;
class QLineEdit;
class QPushButton;

class KviSharedFilesListViewItem : public KviTalListViewItem
{
public:
	KviSharedFilesListViewItem(KviTalListView * lv,KviSharedFile * f);
	~KviSharedFilesListViewItem();
protected:
	KviSharedFile * m_pSharedFilePointer; // THIS IS READ ONLY!
public:
	KviSharedFile * readOnlySharedFilePointer(){ return m_pSharedFilePointer; };
};



class KviSharedFileEditDialog : public QDialog
{ 
	Q_OBJECT
public:
	KviSharedFileEditDialog(QWidget* par,KviSharedFile * f = 0);
	~KviSharedFileEditDialog();

	QDateTimeEdit * m_pExpireDateTimeEdit;
	QLineEdit     * m_pFilePathEdit;
	QLineEdit     * m_pUserMaskEdit;
	QPushButton   * m_pBrowseButton;
	QLineEdit     * m_pShareNameEdit;
	KviStyledCheckBox     * m_pExpireCheckBox;
public:
	KviSharedFile * getResult();
protected slots:
	void okClicked();
	void browse();
};




class KviSharedFilesWindow : public KviWindow , public KviModuleExtension
{
	friend class KviSharedFilesItem;
	Q_OBJECT
public:
	KviSharedFilesWindow(KviModuleExtensionDescriptor * d,KviFrame * lpFrm);
	~KviSharedFilesWindow();
protected:
	QSplitter           * m_pVertSplitter;

	KviTalListView           * m_pListView;

	QPushButton         * m_pRemoveButton;
	QPushButton         * m_pEditButton;
	QPushButton         * m_pAddButton;
public: // Methods
	virtual void die();
protected:
	virtual QPixmap * myIconPtr();
	virtual void fillCaptionBuffers();
	virtual void resizeEvent(QResizeEvent *e);
	virtual void getBaseLogFileName(KviStr &buffer);
public:
	virtual QSize sizeHint() const;
protected slots:
	void fillFileView();
	void sharedFileAdded(KviSharedFile * f);
	void sharedFileRemoved(KviSharedFile * f);
	void enableButtons();
	void removeClicked();
	void addClicked();
	void editClicked();
};

#endif //_SHAREDFILESWINDOW_H_
