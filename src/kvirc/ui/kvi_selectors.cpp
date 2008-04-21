//=============================================================================
//
//   File : kvi_selectors.cpp
//   Creation date : Mon Nov 13 2000 15:22:12 CEST by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2000 Szymon Stefanek (pragma at kvirc dot net)
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
#define _KVI_SELECTORS_CPP_

#include "kvi_selectors.h"
#include "kvi_locale.h"
#include "kvi_options.h"
#include "kvi_mirccntrl.h"
#include "kvi_filedialog.h"
#include "kvi_kvs_script.h"
#include "kvi_tal_popupmenu.h"

#include <QLabel>
#include <QPainter>
#include <QLayout>
#include <QColorDialog>
#include <QPalette>
#include <QFontDialog>


KviBoolSelector::KviBoolSelector(QWidget * par,const QString & txt,bool *pOption,bool bEnabled)
: KviStyledCheckBox(txt,par), KviSelectorInterface()
{
	setEnabled(bEnabled);
	setChecked(*pOption);
	m_pOption = pOption;
}

void KviBoolSelector::setNotEnabled(bool bNotEnabled)
{
	setEnabled(!bNotEnabled);
}

void KviBoolSelector::commit()
{
	*m_pOption = isChecked();
}

KviUIntSelector::KviUIntSelector(QWidget * par,const QString & txt,unsigned int *pOption,
	unsigned int uLowBound,unsigned int uHighBound,unsigned int uDefault,bool bEnabled,bool bShortInt)
: KviTalHBox(par) , KviSelectorInterface()
{
	m_pLabel = new QLabel(txt,this);
	//m_pLineEdit = new QLineEdit(this);
	//m_pLineEdit->setMaximumWidth(150);
	m_pSpinBox = new QSpinBox(this);

	m_bIsShortInt = bShortInt;

	setEnabled(bEnabled);

	m_pOption = pOption;

	m_uLowBound = uLowBound;
	m_uHighBound = uHighBound;
	m_uDefault = uDefault;

	m_pSpinBox->setMinValue(m_uLowBound);
	m_pSpinBox->setMaxValue(m_uHighBound);

	//KviStr tmp(KviStr::Format,"%u",bShortInt ? (unsigned int) *((unsigned short int *)pOption) : *pOption);
	//m_pLineEdit->setText(tmp.ptr());
	m_pSpinBox->setValue(bShortInt ? (unsigned int) *((unsigned short int *)pOption) : *pOption);

	setSpacing(4);
	setStretchFactor(m_pLabel,1);
}

void KviUIntSelector::setPrefix(const QString & txt)
{
	m_pSpinBox->setPrefix(txt);
}

void KviUIntSelector::setSuffix(const QString & txt)
{
	m_pSpinBox->setSuffix(txt);
}

void KviUIntSelector::commit()
{
	KviStr tmp = m_pSpinBox->cleanText();
	bool bOk;
	unsigned int val = tmp.toUInt(&bOk);
	if(!bOk)val = m_uDefault;
	if(m_uHighBound > m_uLowBound)
	{
		if(val < m_uLowBound)val = m_uLowBound;
		else if(val > m_uHighBound)val = m_uHighBound;
	}

	if(m_bIsShortInt)*((unsigned short int *)m_pOption) = (unsigned short int)val;
	else *m_pOption = val;
}

void KviUIntSelector::setEnabled(bool bEnabled)
{
	KviTalHBox::setEnabled(bEnabled);
	m_pLabel->setEnabled(bEnabled);
	m_pSpinBox->setEnabled(bEnabled);
}


KviStringSelector::KviStringSelector(QWidget * par,const QString & txt,QString * pOption,bool bEnabled)
: KviTalHBox(par) , KviSelectorInterface()
{
	m_pLabel = new QLabel(txt,this);
	m_pLineEdit = new QLineEdit(this);
	//m_pLineEdit->setMinimumWidth(200);
	QString tmp = *pOption;
	m_pLineEdit->setText(tmp);

	setSpacing(4);
	setStretchFactor(m_pLineEdit,1);

	m_pOption = pOption;

	setEnabled(bEnabled);
}

KviStringSelector::~KviStringSelector()
{
}

void KviStringSelector::commit()
{
	QString tmp = m_pLineEdit->text();
	*m_pOption = tmp;
}

void KviStringSelector::setEnabled(bool bEnabled)
{
	KviTalHBox::setEnabled(bEnabled);
	m_pLineEdit->setEnabled(bEnabled);
	m_pLabel->setEnabled(bEnabled);
}

void KviStringSelector::setText(const QString& text){
	m_pLineEdit->setText(text);
}

KviPasswordSelector::KviPasswordSelector(QWidget * par,const QString & txt,QString *pOption,bool bEnabled)
: KviStringSelector(par,txt,pOption,bEnabled)
{
	m_pLineEdit->setEchoMode(QLineEdit::Password);
}


KviPixmapPreview::KviPixmapPreview(QWidget * par)
: KviTalScrollView(par)
{
	m_pPixmap = 0;
	resizeContents(0,0);
}

KviPixmapPreview::~KviPixmapPreview()
{
}

void KviPixmapPreview::setPixmap(KviPixmap * pix)
{
	m_pPixmap = pix;
	if(m_pPixmap)
	{
		if(m_pPixmap->pixmap())
		{
			resizeContents(m_pPixmap->pixmap()->width(),m_pPixmap->pixmap()->height());
			update();
			return;
		}
	}
	resizeContents(0,0);
	update();
}

void KviPixmapPreview::drawContents(QPainter * p, int clipx, int clipy, int clipw, int cliph)
{
	if(m_pPixmap)
	{
		if(m_pPixmap->pixmap())
		{
			p->drawPixmap(clipx,clipy,*(m_pPixmap->pixmap()),clipx,clipy,clipw,cliph);
		}
	}
}


KviPixmapSelector::KviPixmapSelector(QWidget * par,const QString & txt,KviPixmap * pOption,bool bEnabled)
: QWidget(par), KviSelectorInterface()
{
	QGridLayout * g = new QGridLayout(this,3,2,4,8);
	m_pOption = pOption;

	m_localPixmap = *pOption;

	m_pCheckBox = new KviStyledCheckBox(txt,this);
	m_pCheckBox->setChecked(m_localPixmap.pixmap());
	connect(m_pCheckBox,SIGNAL(toggled(bool)),this,SLOT(checkBoxToggled(bool)));
	g->addMultiCellWidget(m_pCheckBox,0,0,0,1);

	m_pPreview = new KviPixmapPreview(this);
	m_pPreview->setPixmap(&m_localPixmap);
	g->addMultiCellWidget(m_pPreview,1,1,0,1);

	m_pFileNameLabel = new QLabel(this);
	m_pFileNameLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
	if(m_localPixmap.pixmap())m_pFileNameLabel->setText(m_localPixmap.path());
	g->addWidget(m_pFileNameLabel,2,0);

	m_pChooseButton = new QPushButton("...",this);
	g->addWidget(m_pChooseButton,2,1);
	connect(m_pChooseButton,SIGNAL(clicked()),this,SLOT(choosePixmap()));

	g->setRowStretch(1,1);
	g->setColumnStretch(0,1);

	setEnabled(bEnabled);
}

KviPixmapSelector::~KviPixmapSelector()
{
}


void KviPixmapSelector::checkBoxToggled(bool bEnabled)
{
	setEnabled(isEnabled());
}

void KviPixmapSelector::commit()
{
	if(m_pCheckBox->isChecked())
	{
		*m_pOption = m_localPixmap;
	} else {
		*m_pOption = KviPixmap(); // null pixmap
	}
}

void KviPixmapSelector::choosePixmap()
{
//	KviStr tmp;
 	QString tmp;
	if(KviFileDialog::askForOpenFileName(tmp,__tr("Choose an Image File - KVIrc")))
	{
		setImagePath(tmp);
	}
}

void KviPixmapSelector::setImagePath(const char * path)
{
	m_localPixmap.load(path);
	m_pPreview->setPixmap(&m_localPixmap);

	if(m_localPixmap.isNull())
	{
		KviStr tmp2(KviStr::Format,__tr("Unloadable: %s"),path);
		m_pFileNameLabel->setText(tmp2.ptr());
		m_pCheckBox->setChecked(false);
	} else {
		m_pCheckBox->setChecked(true);
		m_pFileNameLabel->setText(path);
	}

	setEnabled(isEnabled());
}

void KviPixmapSelector::setEnabled(bool bEnabled)
{
	QWidget::setEnabled(bEnabled);
	m_pCheckBox->setEnabled(bEnabled);
	m_pPreview->setEnabled(bEnabled && m_pCheckBox->isChecked());
	m_pFileNameLabel->setEnabled(bEnabled && m_pCheckBox->isChecked());
	m_pChooseButton->setEnabled(bEnabled && m_pCheckBox->isChecked());
}


// FIXME: #warning "Option for DIR_MUST_EXISTS...(this widget could be turned into a file selector too)"
KviFileSelector::KviFileSelector(QWidget * par,const QString & txt,QString * pOption,bool bEnabled,unsigned int uFlags,const QString &szFilter)
: KviTalHBox(par), KviSelectorInterface()
{
	m_uFlags = uFlags;
	m_szFilter = szFilter;
	m_pLabel = new QLabel(txt,this);
	m_pLineEdit = new QLineEdit(this);
	//m_pLineEdit->setMinimumWidth(200);
	m_pLineEdit->setText(*pOption);
	m_pButton = new QPushButton(__tr2qs("&Browse..."),this);
	connect(m_pButton,SIGNAL(clicked()),this,SLOT(browseClicked()));

	setSpacing(4);
	setStretchFactor(m_pLineEdit,1);

	m_pOption = pOption;

	setEnabled(bEnabled);
}

void KviFileSelector::commit()
{
	*m_pOption = m_pLineEdit->text();
}

void KviFileSelector::setEnabled(bool bEnabled)
{
	KviTalHBox::setEnabled(bEnabled);
	m_pLineEdit->setEnabled(bEnabled);
	m_pLabel->setEnabled(bEnabled);
	m_pButton->setEnabled(bEnabled);
}

void KviFileSelector::browseClicked()
{
	select();
}

void KviFileSelector::setSelection(const QString &szSelection)
{
	m_pLineEdit->setText(szSelection);
}

void KviFileSelector::select()
{
	//KviStr tmp;
	QString tmp = *m_pOption;
	if(m_uFlags & ChooseSaveFileName)
	{
 		if(KviFileDialog::askForSaveFileName(tmp,__tr2qs("Choose a File - KVIrc"),tmp,m_szFilter,true,!(m_uFlags & DontConfirmOverwrite)))
		{
			m_pLineEdit->setText(tmp);
			emit selectionChanged(tmp);
		}
	} else {
 		if(KviFileDialog::askForOpenFileName(tmp,__tr2qs("Choose a File - KVIrc"),tmp,m_szFilter,true))
		{
			m_pLineEdit->setText(tmp);
			emit selectionChanged(tmp);
		}
	}

}


KviDirectorySelector::KviDirectorySelector(QWidget * par,const QString & txt,QString * pOption,bool bEnabled)
: KviFileSelector(par,txt,pOption,bEnabled)
{
}

void KviDirectorySelector::select()
{
	QString tmp;
	if(KviFileDialog::askForDirectoryName(tmp,__tr2qs("Choose a Directory - KVIrc"),""))
	{
		m_pLineEdit->setText(tmp);
	}

}


KviStringListSelector::KviStringListSelector(QWidget * par,const QString & txt,QStringList * pOption,bool bEnabled)
: KviTalVBox(par), KviSelectorInterface()
{
	m_pLabel = new QLabel(txt,this);
	m_pListBox = new KviTalListBox(this);
	m_pLineEdit = new QLineEdit(this);
	connect(m_pLineEdit,SIGNAL(textChanged(const QString &)),this,SLOT(textChanged(const QString &)));
	connect(m_pLineEdit,SIGNAL(returnPressed()),this,SLOT(addClicked()));
	KviTalHBox * hBox = new KviTalHBox(this);
	m_pAddButton = new QPushButton(__tr2qs("A&dd"),hBox);
	connect(m_pAddButton,SIGNAL(clicked()),this,SLOT(addClicked()));
	m_pRemoveButton = new QPushButton(__tr2qs("Re&move"),hBox);
	connect(m_pRemoveButton,SIGNAL(clicked()),this,SLOT(removeClicked()));
	m_pOption = pOption;
	m_pListBox->insertStringList(*pOption);
	m_pListBox->setSelectionMode(KviTalListBox::Extended);
	connect(m_pListBox,SIGNAL(selectionChanged()),this,SLOT(selectionChanged()));
	setSpacing(4);
	setStretchFactor(m_pListBox,1);
	setEnabled(bEnabled);
}

KviStringListSelector::~KviStringListSelector()
{
}

void KviStringListSelector::selectionChanged()
{
	unsigned int uCount = m_pListBox->count();
	bool bSomeSelected = false;
	for(unsigned int u=0;u<uCount;u++)
	{
		if(m_pListBox->isSelected(u))
		{
			bSomeSelected = true;
			break;
		}
	}
	m_pRemoveButton->setEnabled(isEnabled() && bSomeSelected);
}

void KviStringListSelector::textChanged(const QString &str)
{
	str.stripWhiteSpace();
	m_pAddButton->setEnabled((str.length() > 0) && isEnabled());
}

void KviStringListSelector::setEnabled(bool bEnabled)
{
	KviTalVBox::setEnabled(bEnabled);
	m_pLineEdit->setEnabled(bEnabled);
	m_pLabel->setEnabled(bEnabled);
	QString txt = m_pLineEdit->text();
	txt.stripWhiteSpace();
	m_pAddButton->setEnabled(bEnabled && (txt.length() > 0));
	unsigned int uCount = m_pListBox->count();
	bool bSomeSelected = false;
	for(unsigned int u=0;u<uCount;u++)
	{
		if(m_pListBox->isSelected(u))
		{
			bSomeSelected = true;
			break;
		}
	}
	m_pRemoveButton->setEnabled(bEnabled && bSomeSelected);
	m_pListBox->setEnabled(bEnabled);
}

void KviStringListSelector::commit()
{
	unsigned int uCount = m_pListBox->count();
	m_pOption->clear();
	for(unsigned int u=0;u<uCount;u++)
	{
		QString str = m_pListBox->text(u);
		str.stripWhiteSpace();
		if(str.length() > 0)m_pOption->append(str);
	}
}

void KviStringListSelector::addClicked()
{
	QString str = m_pLineEdit->text();
	str.stripWhiteSpace();
	if(str.length() > 0)m_pListBox->insertItem(str);
	m_pLineEdit->setText("");
}

void KviStringListSelector::removeClicked()
{
	unsigned int uCount = m_pListBox->count();
	for(unsigned int u=0;u<uCount;u++)
	{
		while(m_pListBox->isSelected(u))m_pListBox->removeItem(u);
	}
}


KviColorSelector::KviColorSelector(QWidget * par,const QString & txt,QColor * pOption,bool bEnabled)
: KviTalHBox(par), KviSelectorInterface()
{
	m_pLabel = new QLabel(txt,this);

	m_pButton = new QPushButton(" ",this);
	// m_pButton->setMinimumWidth(150);
	connect(m_pButton,SIGNAL(clicked()),this,SLOT(changeClicked()));

	setSpacing(4);
	setStretchFactor(m_pLabel,1);

	setButtonPalette(pOption);

	m_pOption = pOption;

	setEnabled(bEnabled);
}

void KviColorSelector::setButtonPalette(QColor * pOption)
{
	QPalette pal(*pOption,colorGroup().background());
	m_memColor = *pOption;
	m_pButton->setPalette(pal);

	QPixmap pix(16,16);
	pix.fill(*pOption);
	m_pButton->setIconSet(pix);
}

void KviColorSelector::forceColor(QColor clr)
{
	setButtonPalette(&clr);
}

void KviColorSelector::changeClicked()
{
	QColor tmp = QColorDialog::getColor(m_memColor);
	if(tmp.isValid())setButtonPalette(&tmp);
}

void KviColorSelector::commit()
{
	*m_pOption = m_memColor;
}

void KviColorSelector::setEnabled(bool bEnabled)
{
	KviTalHBox::setEnabled(bEnabled);
	m_pLabel->setEnabled(bEnabled);
	m_pButton->setEnabled(bEnabled);
}


KviFontSelector::KviFontSelector(QWidget * par,const QString & txt,QFont * pOption,bool bEnabled)
: KviTalHBox(par), KviSelectorInterface()
{
	m_pLabel = new QLabel(txt,this);

	m_pButton = new QPushButton("",this);
	// m_pButton->setMinimumWidth(150);
	connect(m_pButton,SIGNAL(clicked()),this,SLOT(changeClicked()));

	setSpacing(4);
	setStretchFactor(m_pLabel,1);

	setButtonFont(pOption);

	m_pOption = pOption;

	setEnabled(bEnabled);
}

void KviFontSelector::setButtonFont(QFont * pOption)
{
	m_pButton->setText(pOption->family());
	m_pButton->setFont(*pOption);
}

void KviFontSelector::changeClicked()
{
	bool bOk;
	QFont tmp = QFontDialog::getFont(&bOk,m_pButton->font());
	if(bOk)setButtonFont(&tmp);
}

void KviFontSelector::commit()
{
	*m_pOption = m_pButton->font();
}

void KviFontSelector::setEnabled(bool bEnabled)
{
	KviTalHBox::setEnabled(bEnabled);
	m_pLabel->setEnabled(bEnabled);
	m_pButton->setEnabled(bEnabled);
}


KviMircTextColorSelector::KviMircTextColorSelector(QWidget * par,const QString &txt,unsigned int * uFore,unsigned int * uBack,bool bEnabled)
: KviTalHBox(par), KviSelectorInterface()
{
	m_pLabel = new QLabel(txt,this);

	m_pButton = new QPushButton(__tr2qs("Sample Text"),this);
	// m_pButton->setMinimumWidth(150);
	connect(m_pButton,SIGNAL(clicked()),this,SLOT(buttonClicked()));

	setSpacing(4);
	setStretchFactor(m_pLabel,1);

	m_pUFore = uFore;
	m_pUBack = uBack;
	
	m_uBack = *uBack;
	m_uFore = *uFore;

	setButtonPalette();

	setEnabled(bEnabled);
	
	m_pContextPopup = new KviTalPopupMenu(this);
	
	m_pForePopup = new KviTalPopupMenu(this);
	connect(m_pForePopup,SIGNAL(activated(int)),this,SLOT(foreSelected(int)));
	int i;
	for(i=0;i<KVI_MIRCCOLOR_MAX_FOREGROUND;i++)
	{
		QPixmap tmp(120,16);
		tmp.fill(KVI_OPTION_MIRCCOLOR(i));
		int id = m_pForePopup->insertItem(tmp,QString("x"));
		m_pForePopup->setItemParameter(id,i);
	}
	m_pContextPopup->insertItem(__tr2qs("Foreground"),m_pForePopup);

	m_pBackPopup = new KviTalPopupMenu(this);
	connect(m_pBackPopup,SIGNAL(activated(int)),this,SLOT(backSelected(int)));
	i = m_pBackPopup->insertItem(__tr2qs("Transparent"));
	m_pBackPopup->setItemParameter(i,KVI_TRANSPARENT);
	for(i=0;i<KVI_MIRCCOLOR_MAX_BACKGROUND;i++)
	{
		QPixmap tmp(120,16);
		tmp.fill(KVI_OPTION_MIRCCOLOR(i));
		int id = m_pForePopup->insertItem(tmp,QString("x"));
		m_pBackPopup->setItemParameter(id,i);
	}
	m_pContextPopup->insertItem(__tr2qs("Background"),m_pBackPopup);
}

KviMircTextColorSelector::~KviMircTextColorSelector()
{
}

void KviMircTextColorSelector::commit()
{
	*m_pUFore = m_uFore;
	*m_pUBack = m_uBack;
}

void KviMircTextColorSelector::setEnabled(bool bEnabled)
{
	KviTalHBox::setEnabled(bEnabled);
	m_pLabel->setEnabled(bEnabled);
	m_pButton->setEnabled(bEnabled);
}

void KviMircTextColorSelector::setButtonPalette()
{
	QPalette pal;
	
	if(m_uBack > KVI_MIRCCOLOR_MAX_BACKGROUND)
	{
		if(m_uBack != KVI_TRANSPARENT)m_uBack = KVI_TRANSPARENT;
		pal = palette();
	} else {
		pal = QPalette(KVI_OPTION_MIRCCOLOR(m_uBack));
	}

	if(m_uFore > KVI_MIRCCOLOR_MAX_FOREGROUND)m_uFore = KVI_MIRCCOLOR_MAX_FOREGROUND;
	
	pal.setColor(QColorGroup::ButtonText,KVI_OPTION_MIRCCOLOR(m_uFore));
	pal.setColor(QColorGroup::Text,KVI_OPTION_MIRCCOLOR(m_uFore));

	m_pButton->setPalette(pal);
}

void KviMircTextColorSelector::buttonClicked()
{
	QPoint p = m_pButton->mapToGlobal(QPoint(0,m_pButton->height()));
	m_pContextPopup->popup(p);
}

void KviMircTextColorSelector::foreSelected(int id)
{
	if(m_pForePopup)
		m_uFore = m_pForePopup->itemParameter(id);
	setButtonPalette();
}

void KviMircTextColorSelector::backSelected(int id)
{
	if(m_pBackPopup)
		m_uBack = m_pBackPopup->itemParameter(id);
	setButtonPalette();
}

KviSoundSelector::KviSoundSelector(QWidget * par,const QString & txt,QString * pOption,bool bEnabled)
:KviFileSelector(par,txt,pOption,bEnabled)
{
	m_pPlayButton =  new QPushButton(__tr2qs("Play"),this);
	connect(m_pPlayButton,SIGNAL(clicked()),this,SLOT(playSound()));
}

KviSoundSelector::~KviSoundSelector()
{
}

void KviSoundSelector::playSound()
{
	KviKvsScript::run("snd.play $0",0,new KviKvsVariantList(new KviKvsVariant(m_pLineEdit->text())));
}

void KviSoundSelector::setEnabled(bool bEnabled)
{
	KviFileSelector::setEnabled(bEnabled);
	m_pPlayButton->setEnabled(bEnabled);
}

KviChanListViewItem::KviChanListViewItem(KviTalListView* pList,QString szChan,QString szPass)
:KviTalListViewItem(pList,szChan)
{
	m_szPass=szPass;
	QString mask;
	mask.fill('*',szPass.length());
	setText(1,mask);
}

KviCahnnelListSelector::KviCahnnelListSelector(QWidget * par,const QString & txt,QStringList * pOption,bool bEnabled)
: KviTalVBox(par), KviSelectorInterface()
{
	m_pLabel = new QLabel(txt,this);
	m_pListView = new KviTalListView(this);
	m_pListView->addColumn(__tr2qs("Channel name"));
	m_pListView->addColumn(__tr2qs("Channel password"));
	
	KviTalHBox* pEditsHBox = new KviTalHBox(this);
	
	m_pChanLineEdit = new QLineEdit(pEditsHBox);
	connect(m_pChanLineEdit,SIGNAL(textChanged(const QString &)),this,SLOT(textChanged(const QString &)));
	connect(m_pChanLineEdit,SIGNAL(returnPressed()),this,SLOT(addClicked()));
	
	m_pPassLineEdit = new QLineEdit(pEditsHBox);
	m_pPassLineEdit->setEchoMode(QLineEdit::Password);
	connect(m_pPassLineEdit,SIGNAL(textChanged(const QString &)),this,SLOT(textChanged(const QString &)));
	connect(m_pPassLineEdit,SIGNAL(returnPressed()),this,SLOT(addClicked()));
	
	
	KviTalHBox * hBox = new KviTalHBox(this);
	m_pAddButton = new QPushButton(__tr2qs("A&dd"),hBox);
	connect(m_pAddButton,SIGNAL(clicked()),this,SLOT(addClicked()));
	m_pRemoveButton = new QPushButton(__tr2qs("Re&move"),hBox);
	connect(m_pRemoveButton,SIGNAL(clicked()),this,SLOT(removeClicked()));
	m_pOption = pOption;
	
	for ( QStringList::Iterator it = pOption->begin(); it != pOption->end(); ++it ) {
		new KviChanListViewItem(m_pListView,(*it).section(':',0,0),(*it).section(':',1));
	}

	m_pListView->setSelectionMode(KviTalListView::Extended);
	m_pListView->setAllColumnsShowFocus(TRUE);
	connect(m_pListView,SIGNAL(selectionChanged()),this,SLOT(selectionChanged()));
	setSpacing(4);
	setStretchFactor(m_pListView,1);
	setEnabled(bEnabled);
}

KviCahnnelListSelector::~KviCahnnelListSelector()
{
}

void KviCahnnelListSelector::commit()
{
	m_pOption->clear();
	register KviChanListViewItem* pItem;
	KviTalListViewItemIterator it( m_pListView);
	while ( it.current() ) {
		pItem = (KviChanListViewItem*)( it.current() );
		m_pOption->append(pItem->text(0)+":"+pItem->pass());
		++it;
	}
}

void KviCahnnelListSelector::setEnabled(bool bEnabled)
{
	m_pLabel->setEnabled(bEnabled);
	m_pListView->setEnabled(bEnabled);
	m_pChanLineEdit->setEnabled(bEnabled);
	m_pPassLineEdit->setEnabled(bEnabled);
	m_pAddButton->setEnabled(bEnabled);
	m_pRemoveButton->setEnabled(bEnabled);
}

void KviCahnnelListSelector::textChanged(const QString &str)
{
	m_pAddButton->setEnabled(!m_pChanLineEdit->text().isEmpty());
}

void KviCahnnelListSelector::selectionChanged()
{
}

void KviCahnnelListSelector::addClicked()
{
	if(!m_pChanLineEdit->text().isEmpty())
	{
		new KviChanListViewItem(m_pListView,m_pChanLineEdit->text().stripWhiteSpace(),m_pPassLineEdit->text().stripWhiteSpace());
		m_pChanLineEdit->clear();
		m_pPassLineEdit->clear();
	}
}

void KviCahnnelListSelector::removeClicked()
{
	KviPointerList<KviTalListViewItem> lst;
	KviTalListViewItemIterator it( m_pListView, KviTalListViewItemIterator::Selected );
	while ( it.current() ) {
		lst.append((KviTalListViewItem *)it.current() );
		++it;
	}
	lst.setAutoDelete(TRUE);
	lst.clear();
}

#ifndef COMPILE_USE_STANDALONE_MOC_SOURCES
#include "kvi_selectors.moc"
#endif //!COMPILE_USE_STANDALONE_MOC_SOURCES
