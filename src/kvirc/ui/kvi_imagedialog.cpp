//
//   File : kvi_imagedialog.cpp
//   Creation date : Sun Dec 22 2002 19:42 by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2002 Szymon Stefanek (pragma at kvirc dot net)
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

#define __KVIRC__
#include "kvi_imagedialog.h"

#include "kvi_locale.h"

#include <qlayout.h>
#include <qpushbutton.h>

#include <qimage.h>
#include <qfileinfo.h>
#include <qdir.h>

#include <qpainter.h>

#include "kvi_fileutils.h"

#include "kvi_iconmanager.h"
#include "kvi_options.h"

#include "kvi_app.h"

int KviImageDialogItem::height(const QListBox *lb) const
{
	return pixmap()->height() + 12 + lb->fontMetrics().lineSpacing();
}

int KviImageDialogItem::width(const QListBox *lb) const
{
	int w;
	if(text().isEmpty())w = 24;
	w = lb->fontMetrics().width(text()) + 4;
	if(w > 100)w = 100;
	if(w < 24)w = 24;
	return QMAX(pixmap()->width() + 10,w);
}

void KviImageDialogItem::paint(QPainter * p)
{
	const QPixmap *pm = pixmap();
	if(pm && !pm->isNull())p->drawPixmap(5,5, *pm);

	if(!m_bIsFolder)
	{
		p->setPen(Qt::gray);
		p->drawRect(3,3,pm->width() + 4,pm->height() + 4);
	}

	QRect daRect(listBox()->itemRect(this));

	p->setPen(Qt::black);
	p->drawRect(1,1,daRect.width() - 2,daRect.height() - 2);

	if(text().isEmpty())return;

	QString t = text();

	QFontMetrics fm(p->fontMetrics());

	int wdth = fm.width(t);

	int idx = t.length();
	while(wdth > (daRect.width() - 6) && idx > 3)
	{
		t = text();
		t.truncate(idx);
		t.append("...");
		wdth = fm.width(t);
		idx--;
	}

	p->drawText(3,pm->height() + 8,daRect.width() - 6,daRect.height() - (pm->height() + 6),Qt::AlignLeft | Qt::AlignTop,t);
}


KviImageDialog::KviImageDialog(QWidget * par,
		const QString &szCaption,
		int types,
		int initialType,
		const QString &szInitialDir,
		int maxPreviewFileSize,bool modal)
: QDialog(par)
{
	m_szInitialPath = szInitialDir;
	setModal(modal);
	m_iMaxPreviewFileSize = maxPreviewFileSize;

	setCaption(szCaption.isEmpty() ? __tr2qs("Choose image ...") : szCaption);

	m_pTimer = new QTimer(this);
	connect(m_pTimer,SIGNAL(timeout()),this,SLOT(heartbeat()));

	QGridLayout * g = new QGridLayout(this,4,3,5,3);

	m_pTypeComboBox = new QComboBox(this);

	g->addMultiCellWidget(m_pTypeComboBox,0,0,0,2);

	m_pTypeList = new QValueList<int>;

	QString bi = __tr2qs("Builtin images");

	QString tmp = bi;

	if((types & KID_TYPE_ALL) == 0)types = KID_TYPE_FULL_PATH;

	if(types & KID_TYPE_BUILTIN_IMAGES_SMALL)
	{
		tmp += ": ";
		tmp += __tr2qs("Small icons");
		m_pTypeComboBox->insertItem(tmp);
		m_pTypeList->append(KID_TYPE_BUILTIN_IMAGES_SMALL);
	}

	if(types & KID_TYPE_FULL_PATH)
	{
		m_pTypeComboBox->insertItem(__tr2qs("Full path"));
		m_pTypeList->append(KID_TYPE_FULL_PATH);
	}

	int idx = m_pTypeList->findIndex(initialType);
	if(idx < 0)idx = 0;

	QWidget * l = new QWidget(this);
	g->addMultiCellWidget(l,1,1,0,2);


	m_pListBox = new QListBox(this);
	m_pListBox->setColumnMode(QListBox::FitToWidth);
	m_pListBox->setRowMode(QListBox::Variable);

	m_pTip = new KviDynamicToolTip(m_pListBox->viewport());

	g->addMultiCellWidget(m_pListBox,2,2,0,2);

	QPushButton * b = new QPushButton(__tr2qs("Cancel"),this);
	connect(b,SIGNAL(clicked()),this,SLOT(cancelClicked()));
	g->addWidget(b,3,1);

	b = new QPushButton(__tr2qs("Ok"),this);
	connect(b,SIGNAL(clicked()),this,SLOT(okClicked()));
	g->addWidget(b,3,2);

	g->setRowStretch(2,1);
	g->setColStretch(0,1);

	connect(m_pTypeComboBox,SIGNAL(activated(int)),this,SLOT(jobTypeSelected(int)));
	connect(m_pListBox,SIGNAL(doubleClicked(QListBoxItem *)),this,SLOT(itemDoubleClicked(QListBoxItem *)));
	connect(m_pTip,SIGNAL(tipRequest(KviDynamicToolTip *,const QPoint &)),this,SLOT(tipRequest(KviDynamicToolTip *,const QPoint &)));

	m_pTypeComboBox->setCurrentItem(idx);
	jobTypeSelected(idx);

	m_pListBox->setMinimumSize(420,350);
}

KviImageDialog::~KviImageDialog()
{
	delete m_pTimer;
	delete m_pTypeList;
}

void KviImageDialog::jobTypeSelected(int index)
{
	if(index < 0)return;
	if(index >= (int)(m_pTypeList->count()))index = (int)m_pTypeList->count();
	if(m_szInitialPath.isEmpty())
		startJob(*(m_pTypeList->at(index)),KVI_OPTION_STRING(KviOption_stringLastImageDialogPath));
	else {
		startJob(*(m_pTypeList->at(index)),m_szInitialPath);
		m_szInitialPath = ""; // clear it so we will use the last path 
	}
}

void KviImageDialog::startJob(int type,const QString &szInitialPath)
{
	m_pTimer->stop();
	m_iJobType = type;

	m_iJobIndexHelper = 0;
	if(m_iJobType == KID_TYPE_FULL_PATH)
	{
		QDir d(szInitialPath);
		if(!d.exists())d = QDir::homeDirPath();
		if(!d.exists())d = QDir::rootDirPath();
		m_szJobPath = d.absPath();
		KVI_OPTION_STRING(KviOption_stringLastImageDialogPath) = m_szJobPath;
		m_lJobFileList = d.entryList(QDir::Hidden | QDir::All,QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
	}

	m_pTimer->start(100);
}

void KviImageDialog::jobTerminated()
{
	m_pTimer->stop();
}

void KviImageDialog::heartbeat()
{
	if(m_iJobIndexHelper == 0)m_pListBox->clear();


	switch(m_iJobType)
	{
		case KID_TYPE_BUILTIN_IMAGES_SMALL:
		{
			if(m_iJobIndexHelper >= KVI_NUM_SMALL_ICONS)
			{
				jobTerminated();
				return;
			}
			int max = m_iJobIndexHelper + 15;
			if(max > KVI_NUM_SMALL_ICONS)max = KVI_NUM_SMALL_ICONS;
			while(m_iJobIndexHelper < max)
			{
				QString id = g_pIconManager->getSmallIconName(m_iJobIndexHelper);
				KviImageDialogItem * it;
				QString tip;
				KviQString::sprintf(tip,__tr2qs("Builtin $icon(%Q) [index %d]"),&id,m_iJobIndexHelper);
				QString image_id = "$icon(";
				image_id += id;
				image_id += ")";
				it = new KviImageDialogItem(m_pListBox,*(g_pIconManager->getSmallIcon(m_iJobIndexHelper)),id,image_id,tip);
				m_iJobIndexHelper++;
			}
		}
		break;
		case KID_TYPE_FULL_PATH:
		{
			m_iJobIndexHelper++;
			if(m_lJobFileList.isEmpty())
			{
				jobTerminated();
				return;
			}
			int idx = 0;
			while((idx < 20) && (!m_lJobFileList.isEmpty()))
			{
				QString szFile = m_lJobFileList.first();
				m_lJobFileList.remove(szFile);
				QString szPath = m_szJobPath;
				szPath += KVI_PATH_SEPARATOR;
				szPath += szFile;
				QFileInfo fi(szPath);
				idx += fi.size() / 128000; // we do less entries when have big files to read
				if(fi.isDir())
				{
					if(szFile != ".")
					{
						QString tip = szFile;
						tip += "<br><hr>";
						tip += __tr2qs("directory");
						KviImageDialogItem * it;
						it = new KviImageDialogItem(m_pListBox,*(g_pIconManager->getBigIcon(KVI_BIGICON_FOLDER)),szFile,szPath,tip,true);
					}
				} else {
					if(((int)fi.size()) < m_iMaxPreviewFileSize)
					{
						QImage i(szPath);
						if(i.isNull())continue;
						QPixmap pix;
						if((i.width() > 80) || (i.height() > 80))pix = i.scale(80,80,QImage::ScaleMin);
						else pix = i;

						QString tip = szFile;
						tip += "<br><hr>";
						QString sz;
						sz.setNum(i.width());
						tip += sz;
						tip += " x ";
						sz.setNum(i.height());
						tip += sz;
						tip += " ";
						tip += __tr2qs("pixels");
						tip += "<br>";
						sz.setNum(fi.size());
						tip += sz;
						tip += " ";
						tip += __tr2qs("bytes");
						tip += "<br>";

						KviImageDialogItem * it;
						it = new KviImageDialogItem(m_pListBox,pix,szFile,szPath,tip);
					}
				}
				idx++;
			}
		}
		break;
	}
}

void KviImageDialog::okClicked()
{
	QListBoxItem * it = 0;
	int idx = m_pListBox->currentItem();
	if(idx != -1)it = m_pListBox->item(idx);
	if(!it)return;
	itemDoubleClicked(it);
}

void KviImageDialog::cancelClicked()
{
	m_szSelectedImage = QString::null;
	reject();
}

void KviImageDialog::closeEvent(QCloseEvent * e)
{
	m_szSelectedImage = QString::null;
	QDialog::closeEvent(e);
}

void KviImageDialog::itemDoubleClicked(QListBoxItem * it)
{
	if(!it)return;
	KviImageDialogItem * i = (KviImageDialogItem *)it;
	if(i->isFolder())
	{
		startJob(KID_TYPE_FULL_PATH,i->imageId());
	} else {
		QString szImageId = i->imageId();
		if(szImageId.length() > 0)
		{
			if(szImageId.at(0) == QChar('$'))
				m_szSelectedImage = szImageId; // it's $icon(something)
			else
				g_pApp->mapImageFile(m_szSelectedImage,i->imageId()); // it's a file and we need to map it to our filesystem view
			accept();
		}
	}
}

void KviImageDialog::tipRequest(KviDynamicToolTip *,const QPoint &pnt)
{
	QListBoxItem * it = m_pListBox->itemAt(pnt);
	if(!it)return;
	QRect r = m_pListBox->itemRect(it);
	KviImageDialogItem * i = (KviImageDialogItem *)it;
	m_pTip->tip(r,i->tipText());
}
