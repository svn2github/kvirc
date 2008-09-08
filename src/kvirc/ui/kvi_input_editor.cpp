//============================================================================
//
//   File : kvi_input_editor.cpp
//   Creation date : Fri Sep 5 2008 17:26:34 by Elvio Basello
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2008 Szymon Stefanek (pragma at kvirc dot net)
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
//============================================================================
//   This file was originally part of kvi_input.h
//============================================================================

#include "kvi_input.h"
#include "kvi_input_editor.h"
#include "kvi_input_history.h"
#include "kvi_app.h"
#include "kvi_out.h"
#include "kvi_console.h"
#include "kvi_locale.h"
#include "kvi_options.h"
#include "kvi_ircview.h"
#include "kvi_colorwin.h"
#include "kvi_mirccntrl.h"
#include "kvi_userinput.h"
#include "kvi_kvs_script.h"
#include "kvi_kvs_kernel.h"
#include "kvi_userlistview.h"
#include "kvi_doublebuffer.h"
#include "kvi_texticonwin.h"
#include "kvi_texticonmanager.h"
#include "kvi_tal_popupmenu.h"

#include <QClipboard>
#include <QLabel>
#include <QUrl>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QPixmap>
#include <QFileDialog>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QDragEnterEvent>


// from kvi_input.cpp
extern QFontMetrics            * g_pLastFontMetrics;

// from kvi_app.cpp
extern KviTalPopupMenu         * g_pInputPopup;
extern KviTextIconWindowWidget * g_pTextIconWindow;
extern KviColorWindow          * g_pColorWindow;
extern QPixmap                 * g_pShadedChildGlobalDesktopBackground;

static int g_iInputFontCharWidth[256];
static bool g_bInputFontMetricsDirty = true;


KviInputEditor::KviInputEditor(QWidget * pPar, KviWindow * pWnd, KviUserListView * pView)
:QFrame(pPar)
{
	setObjectName("input_frame");
	m_pIconMenu            = 0;
	m_pInputParent         = pPar;
	m_iMaxBufferSize       = KVI_INPUT_MAX_BUFFER_SIZE;
	m_iCursorPosition      = 0;                             //Index of the char AFTER the cursor
	m_iFirstVisibleChar    = 0;                             //Index of the first visible character
	m_iSelectionBegin      = -1;                            //Index of the first char in the selection
	m_iSelectionEnd        = -1;                            //Index of the last char in the selection
	m_bIMComposing         = false;                         //Whether the input method is active (composing).
	// for input method support
	m_iIMStart             = 0;                             //Index of the start of the preedit string.
	m_iIMLength            = 0;                             //Length of the preedit string.
	m_iIMSelectionBegin    = 0;                             //Index of the start of the selection in preedit string.
	m_iIMSelectionLength   = 0;                             //Length of the selection in preedit string.

	m_bCursorOn            = false;                         //Cursor state
	m_iCursorTimer         = 0;                             //Timer that iverts the cursor state
	m_iDragTimer           = 0;                             //Timer for drag selection updates
	m_iLastCursorXPosition = KVI_INPUT_MARGIN;              //Calculated in paintEvent
	m_iSelectionAnchorChar = -1;                            //Character clicked at the beginning of the selection process
	m_iCurHistoryIdx       = -1;                            //No data in the history
	m_bUpdatesEnabled      = true;
	m_pKviWindow           = pWnd;
	m_pUserListView        = pView;
	m_pHistory             = new KviPointerList<QString>;
	m_pHistory->setAutoDelete(true);
	m_bReadOnly = FALSE;

	setAttribute(Qt::WA_InputMethodEnabled, true);

	setAutoFillBackground(false);
	setFocusPolicy(Qt::StrongFocus);
	setAcceptDrops(true);
	setFrameStyle( StyledPanel );
	setFrameShadow( Plain );

	m_pIconMenu = new KviTalPopupMenu();
	connect(m_pIconMenu,SIGNAL(activated(int)),this,SLOT(iconPopupActivated(int)));

	setCursor(Qt::IBeamCursor);
}

KviInputEditor::~KviInputEditor()
{
	if(g_pLastFontMetrics)
		delete g_pLastFontMetrics;
	g_pLastFontMetrics = 0;
	
	if(m_pIconMenu)
		delete m_pIconMenu;

	delete m_pHistory;

	if(m_iCursorTimer)
		killTimer(m_iCursorTimer);
	killDragTimer();
}

void KviInputEditor::recalcFontMetrics()
{
	QFontMetrics fm(KVI_OPTION_FONT(KviOption_fontInput));
	unsigned short u;
	for(u=1; u<32; u++)
	{
		QChar c = getSubstituteChar(u);
		g_iInputFontCharWidth[u] = fm.width(c);
		if(c != QChar(u))
			g_iInputFontCharWidth[u] += 4;
	}

	for(u=32; u<256; u++)
	{
		g_iInputFontCharWidth[u] = fm.width(QChar(u));
	}
	g_bInputFontMetricsDirty = false;
}

void KviInputEditor::applyOptions()
{
	g_bInputFontMetricsDirty = true;
	update();
}

void KviInputEditor::dragEnterEvent(QDragEnterEvent * e)
{
	if(e->mimeData()->hasUrls())
		e->acceptProposedAction();
}

void KviInputEditor::dropEvent(QDropEvent * e)
{
	QList<QUrl> list;
	if(e->mimeData()->hasUrls())
	{
		list = e->mimeData()->urls();
		//debug("Local files decoded");
		if(!list.isEmpty())
		{
			//debug("List not empty");
			QList<QUrl>::Iterator it = list.begin();
			for( ; it != list.end(); ++it )
			{
				QUrl url = *it;
				QString szPath = url.path();
#if !defined(COMPILE_ON_WINDOWS) && !defined(COMPILE_ON_MINGW)
				if(szPath.length() > 0)
				{
					if(szPath[0] != QChar('/'))
						szPath.prepend("/"); //HACK HACK HACK for Qt bug (?!?)
				}
#endif
				szPath.prepend("/PARSE \"");
				szPath.append("\"");
				if(m_pKviWindow)
					KviKvsScript::run(szPath,m_pKviWindow);
			}
		}
	}
}

int  KviInputEditor::heightHint() const
{
	return sizeHint().height();
}

QSize KviInputEditor::sizeHint() const
{
	//grabbed from qlineedit.cpp
	ensurePolished();
	QFontMetrics fm(KVI_OPTION_FONT(KviOption_fontInput));
	int h = qMax(fm.lineSpacing(), 14) + 2*2; /* innerMargin */
	int w = fm.width( 'x' ) * 17; // "some"
	int m = frameWidth() * 2;
	QStyleOption opt;
	opt.initFrom(this);
	return (style()->sizeFromContents(QStyle::CT_LineEdit,&opt,
		QSize( w + m, h + m ).
		expandedTo(QApplication::globalStrut()),this));
}

void KviInputEditor::paintEvent(QPaintEvent * e)
{
	QPainter p(this);
	SET_ANTI_ALIASING(p);
	drawFrame(&p);
	drawContents(&p);
}

void KviInputEditor::drawContents(QPainter * p)
{
	if(!isVisible())return;

	QRect rect = contentsRect();
	int iWidgetWidth  = rect.width();
	int iWidgetHeight = rect.height();

	KviDoubleBuffer doublebuffer(iWidgetWidth,iWidgetHeight);
	QPixmap * pDoubleBufferPixmap = doublebuffer.pixmap();

	QPainter pa(pDoubleBufferPixmap);
	SET_ANTI_ALIASING(pa);

	pa.setFont(KVI_OPTION_FONT(KviOption_fontInput));

	QFontMetrics fm(pa.fontMetrics());

	if(!g_pLastFontMetrics)
		g_pLastFontMetrics = new QFontMetrics(pa.fontMetrics());

	if(g_bInputFontMetricsDirty)
		recalcFontMetrics();

#ifdef COMPILE_PSEUDO_TRANSPARENCY
	if(g_pShadedChildGlobalDesktopBackground)
	{
		QPoint pnt = mapToGlobal(rect.topLeft());
		pa.drawTiledPixmap(0,0,iWidgetWidth,iWidgetHeight,*g_pShadedChildGlobalDesktopBackground,pnt.x(),pnt.y());
	} else {
#endif
		QPixmap * pPix = KVI_OPTION_PIXMAP(KviOption_pixmapInputBackground).pixmap();

		pa.fillRect(0,0,iWidgetWidth,iWidgetHeight,KVI_OPTION_COLOR(KviOption_colorInputBackground));
		if(pPix)
			KviPixmapUtils::drawPixmapWithPainter(&pa,pPix,KVI_OPTION_UINT(KviOption_uintInputPixmapAlign),rect,iWidgetWidth,iWidgetHeight);
#ifdef COMPILE_PSEUDO_TRANSPARENCY
	}
#endif

	int iCurXPos     = KVI_INPUT_MARGIN;
	int iMaxXPos     = iWidgetWidth-2*KVI_INPUT_MARGIN;
	m_iCurBack       = KVI_INPUT_DEF_BACK; //transparent
	m_iCurFore       = KVI_INPUT_DEF_FORE; //normal fore color
	m_bCurBold       = false;
	m_bCurUnderline  = false;

	int iBottom       = iWidgetHeight-(iWidgetHeight-fm.height())/2;
	int iTextBaseline = fm.ascent()+(iWidgetHeight-fm.height())/2;
	int iTop          = (iWidgetHeight-fm.height())/2;

	runUpToTheFirstVisibleChar();

	int iCharIdx      = m_iFirstVisibleChar;

	pa.setClipRect(0,0,iWidgetWidth,iWidgetHeight);

	//Control the selection state
	if((m_iSelectionEnd < m_iSelectionBegin) || (m_iSelectionEnd == -1) || (m_iSelectionBegin == -1))
	{
		m_iSelectionEnd = -1;
		m_iSelectionBegin = -1;
	}

	if((m_iSelectionBegin != -1) && (m_iSelectionEnd >= m_iFirstVisibleChar))
	{
		int iSelStart = m_iSelectionBegin;

		// TODO Refactor: write a function to combine this with the code determining iIMStart and iIMSelectionStart
		if(iSelStart < m_iFirstVisibleChar)
			iSelStart = m_iFirstVisibleChar;
		int iXLeft = xPositionFromCharIndex(fm,iSelStart,TRUE);
		int iXRight = xPositionFromCharIndex(fm,m_iSelectionEnd + 1,TRUE);

//		pa.setRasterOp(Qt::NotROP);
		pa.fillRect(iXLeft,frameWidth(),iXRight - iXLeft,iWidgetWidth,KVI_OPTION_COLOR(KviOption_colorInputSelectionBackground));
//		pa.setRasterOp(Qt::CopyROP);
	}

	// When m_bIMComposing is true, the text between m_iIMStart and m_iIMStart+m_iIMLength should be highlighted to show that this is the active
	// preedit area for the input method, and the text outside cannot be edited while
	// composing. Maybe this can be implemented similarly as painting the selection?
	// Also notice that inside the preedit, there can also be a selection, given by
	// m_iSelectionBegin and m_iSelectionLength, and the widget needs to highlight that
	// while in IM composition mode
	if(m_bIMComposing && m_iIMLength > 0)
	{
		// TODO Write a function to combine IM selection drawing code. maybe the preedit area too.
		int iIMSelectionStart = m_iIMSelectionBegin;
		if(iIMSelectionStart < m_iFirstVisibleChar)
			iIMSelectionStart = m_iFirstVisibleChar;

		int iXIMSelectionLeft = xPositionFromCharIndex(fm,iIMSelectionStart,TRUE);
		int iXIMSelectionRight = xPositionFromCharIndex(fm,iIMSelectionStart + m_iIMSelectionLength,TRUE);
//		pa.setRasterOp(Qt::NotROP);
		pa.fillRect(iXIMSelectionLeft,0,iXIMSelectionRight - iXIMSelectionLeft, iWidgetWidth,KVI_OPTION_COLOR(KviOption_colorInputSelectionBackground));
//		pa.setRasterOp(Qt::CopyROP);

		// highlight the IM selection
		int iIMStart = m_iIMStart;
		if(m_iIMStart < m_iFirstVisibleChar) m_iIMStart = m_iFirstVisibleChar;
		int xIMLeft = xPositionFromCharIndex(fm,iIMStart,TRUE);
		int xIMRight = xPositionFromCharIndex(fm,iIMStart + m_iIMLength,TRUE);

		// underline the IM preedit
		// Maybe should be put in drawTextBlock, similar to drawing underlined text
		pa.drawLine(xIMLeft, iBottom, xIMRight, iBottom);
	}

	pa.setClipping(false);

	while((iCharIdx < ((int)(m_szTextBuffer.length()))) && (iCurXPos < iMaxXPos))
	{
		extractNextBlock(iCharIdx,fm,iCurXPos,iMaxXPos);

		if(m_bControlBlock)
		{
			pa.setPen(KVI_OPTION_COLOR(KviOption_colorInputControl));

			QString s = getSubstituteChar(m_szTextBuffer[iCharIdx].unicode());

			// the block width is 4 pixels more than the actual character

			pa.drawText(iCurXPos + 2,iTextBaseline,s.left(1));

			pa.drawRect(iCurXPos,iTop,m_iBlockWidth-1,iBottom);
		} else {
			if(m_iSelectionBegin!=-1)
			{
				int iBlockEnd = iCharIdx + m_iBlockLen;
				//block is selected (maybe partially)
				if( iBlockEnd > m_iSelectionBegin && iCharIdx <= m_iSelectionEnd )
				{
					int iSubStart,iSubLen;
					//in common it consists of 3 parts: unselected-selected-unselected
					//some of thst parts can be empty (for example block is fully selected)

					//first part start is always equal to the block start
					iSubStart = iCharIdx;
					iSubLen = m_iSelectionBegin > iCharIdx ? m_iSelectionBegin-iCharIdx : 0;

					if(iSubLen)
					{
						drawTextBlock(&pa,fm,iCurXPos,iTextBaseline,iSubStart,iSubLen,FALSE);
						iCurXPos += m_iBlockWidth;
						m_iBlockWidth=0;
					}

					//second one
					iSubStart += iSubLen;
					iSubLen = m_iSelectionEnd<iBlockEnd ? m_iSelectionEnd-iSubStart+1 : iBlockEnd-iSubStart;

					if(iSubLen)
					{
						drawTextBlock(&pa,fm,iCurXPos,iTextBaseline,iSubStart,iSubLen,TRUE);
						iCurXPos += m_iBlockWidth;
						m_iBlockWidth=0;
					}

					if(m_iSelectionEnd<(iBlockEnd-1))
					{
						iSubStart += iSubLen;
						iSubLen = iBlockEnd-iSubStart;
						drawTextBlock(&pa,fm,iCurXPos,iTextBaseline,iSubStart,iSubLen,FALSE);
					}
				} else {
					drawTextBlock(&pa,fm,iCurXPos,iTextBaseline,iCharIdx,m_iBlockLen);
				}
			} else {
				drawTextBlock(&pa,fm,iCurXPos,iTextBaseline,iCharIdx,m_iBlockLen);
			}
		}

		iCurXPos += m_iBlockWidth;
		iCharIdx += m_iBlockLen;
	}

	//Now the cursor
	m_iLastCursorXPosition = KVI_INPUT_MARGIN;
	m_iBlockLen = m_iFirstVisibleChar;

	while(m_iBlockLen < m_iCursorPosition)
	{
		QChar c = m_szTextBuffer.at(m_iBlockLen);
		m_iLastCursorXPosition+= c.unicode() < 32 ? fm.width(getSubstituteChar(c.unicode())) + 3 : fm.width(c);
		m_iBlockLen++;
	}

	//m_iLastCursorXPosition = cur1XPos;

	if(m_bCursorOn)
	{
		pa.setPen(KVI_OPTION_COLOR(KviOption_colorInputCursor));
		pa.drawLine(m_iLastCursorXPosition,0,m_iLastCursorXPosition,iWidgetHeight);
	} else {
		pa.setPen(KVI_OPTION_COLOR(KviOption_colorInputForeground));
	}

	// The other version of drawPixmap seems to be buggy
	p->drawPixmap(rect.x(),rect.y(),rect.width(),rect.height(),*pDoubleBufferPixmap,0,0,iWidgetWidth,iWidgetHeight);
}

void KviInputEditor::drawTextBlock(QPainter * pa, QFontMetrics & fm, int iCurXPos, int iTextBaseline, int iCharIdx, int iLen, bool bSelected)
{
	QString szTmp = m_szTextBuffer.mid(iCharIdx,iLen);
	m_iBlockWidth = fm.width(szTmp);

	QRect rect = contentsRect();
	int iWidgetHeight = rect.height();

	if(m_iCurFore == KVI_INPUT_DEF_FORE)
	{
		pa->setPen( bSelected ? KVI_OPTION_COLOR(KviOption_colorInputSelectionForeground) : KVI_OPTION_COLOR(KviOption_colorInputForeground));
	} else {
		if(((unsigned char)m_iCurFore) > 16)
		{
			pa->setPen(KVI_OPTION_COLOR(KviOption_colorInputBackground));
		} else {
			pa->setPen(KVI_OPTION_MIRCCOLOR((unsigned char)m_iCurFore));
		}
	}

	if(m_iCurBack != KVI_INPUT_DEF_BACK)
	{
		if(((unsigned char)m_iCurBack) > 16)
		{
			pa->fillRect(iCurXPos,(iWidgetHeight-fm.height())/2,m_iBlockWidth,fm.height(),KVI_OPTION_COLOR(KviOption_colorInputForeground));
		} else {
			pa->fillRect(iCurXPos,(iWidgetHeight-fm.height())/2,m_iBlockWidth,fm.height(),KVI_OPTION_MIRCCOLOR((unsigned char)m_iCurBack));
		}
	}

	pa->drawText(iCurXPos,iTextBaseline,szTmp);

	if(m_bCurBold)pa->drawText(iCurXPos+1,iTextBaseline,szTmp);
	if(m_bCurUnderline)
	{
		pa->drawLine(iCurXPos,iTextBaseline + fm.descent(),iCurXPos+m_iBlockWidth,iTextBaseline + fm.descent());
	}
}

QChar KviInputEditor::getSubstituteChar(unsigned short uControlCode)
{
	switch(uControlCode)
	{
		case KVI_TEXT_COLOR:
			return QChar('K');
			break;
		case KVI_TEXT_BOLD:
			return QChar('B');
			break;
		case KVI_TEXT_RESET:
			return QChar('O');
			break;
		case KVI_TEXT_REVERSE:
			return QChar('R');
			break;
		case KVI_TEXT_UNDERLINE:
			return QChar('U');
			break;
		case KVI_TEXT_CRYPTESCAPE:
			return QChar('P');
			break;
		case KVI_TEXT_ICON:
			return QChar('I');
			break;
		default:
			return QChar(uControlCode);
			break;
	}
}

void KviInputEditor::extractNextBlock(int iIdx, QFontMetrics & fm, int iCurXPos, int iMaxXPos)
{
	m_iBlockLen = 0;
	m_iBlockWidth = 0;

	QChar c = m_szTextBuffer[iIdx];

	if((c.unicode() > 32) ||
		((c != QChar(KVI_TEXT_COLOR)) &&
		(c != QChar(KVI_TEXT_BOLD)) &&
		(c != QChar(KVI_TEXT_UNDERLINE)) &&
		(c != QChar(KVI_TEXT_RESET)) &&
		(c != QChar(KVI_TEXT_REVERSE)) &&
		(c != QChar(KVI_TEXT_CRYPTESCAPE)) &&
		(c != QChar(KVI_TEXT_ICON))))
	{
		m_bControlBlock = false;
		//Not a control code...run..
		while((iIdx < ((int)(m_szTextBuffer.length()))) && (iCurXPos < iMaxXPos))
		{
			c = m_szTextBuffer[iIdx];
			if((c.unicode() > 32) ||
				((c != QChar(KVI_TEXT_COLOR)) &&
				(c != QChar(KVI_TEXT_BOLD)) &&
				(c != QChar(KVI_TEXT_UNDERLINE)) &&
				(c != QChar(KVI_TEXT_RESET)) &&
				(c != QChar(KVI_TEXT_REVERSE)) &&
				(c != QChar(KVI_TEXT_CRYPTESCAPE)) &&
				(c != QChar(KVI_TEXT_ICON))))
			{
				m_iBlockLen++;
				int iXxx = c.unicode() < 32 ? fm.width(getSubstituteChar(c.unicode())) + 3 : fm.width(c);;
				m_iBlockWidth += iXxx;
				iCurXPos      += iXxx;
				iIdx++;
			} else break;
		}
		return;
	} else {
		m_bControlBlock = true;
		m_iBlockLen = 1;
		m_iBlockWidth = g_iInputFontCharWidth[c.unicode()];
		//Control code
		switch(c.unicode())
		{
			case KVI_TEXT_BOLD:
				m_bCurBold = ! m_bCurBold;
				break;
			case KVI_TEXT_UNDERLINE:
				m_bCurUnderline = ! m_bCurUnderline;
				break;
			case KVI_TEXT_RESET:
				m_iCurFore = KVI_INPUT_DEF_FORE;
				m_iCurBack = KVI_INPUT_DEF_BACK;
				m_bCurBold = false;
				m_bCurUnderline = false;
				break;
			case KVI_TEXT_REVERSE:
			{
				char cAuxClr = m_iCurFore;
				m_iCurFore  = m_iCurBack;
				m_iCurBack  = cAuxClr;
			}
			break;
			case KVI_TEXT_CRYPTESCAPE:
			case KVI_TEXT_ICON:
				// makes a single block
				break;
			case KVI_TEXT_COLOR:
			{
				iIdx++;
				if(iIdx >= ((int)(m_szTextBuffer.length())))
					return;

				unsigned char uFore;
				unsigned char uBack;
				iIdx = getUnicodeColorBytes(m_szTextBuffer,iIdx,&uFore,&uBack);
				if(uFore != KVI_NOCHANGE)
				{
					m_iCurFore = uFore;
					if(uBack != KVI_NOCHANGE)
						m_iCurBack = uBack;
				} else {
					// ONLY a CTRL+K
					m_iCurBack = KVI_INPUT_DEF_BACK;
					m_iCurFore = KVI_INPUT_DEF_FORE;
				}
			}
			break;
			default:
				debug("Ops..");
				exit(0);
			break;
		}
	}
}

void KviInputEditor::runUpToTheFirstVisibleChar()
{
	register int iIdx = 0;
	while(iIdx < m_iFirstVisibleChar)
	{
		unsigned short uChar = m_szTextBuffer[iIdx].unicode();
		if(uChar < 32)
		{
			switch(uChar)
			{
				case KVI_TEXT_BOLD:
					m_bCurBold = ! m_bCurBold;
					break;
				case KVI_TEXT_UNDERLINE:
					m_bCurUnderline = ! m_bCurUnderline;
					break;
				case KVI_TEXT_RESET:
					m_iCurFore = KVI_INPUT_DEF_FORE;
					m_iCurBack = KVI_INPUT_DEF_BACK;
					m_bCurBold = false;
					m_bCurUnderline = false;
					break;
				case KVI_TEXT_REVERSE:
				{
					char cAuxClr = m_iCurFore;
					m_iCurFore = m_iCurBack;
					m_iCurBack = cAuxClr;
				}
				break;
				case KVI_TEXT_COLOR:
				{
					iIdx++;
					if(iIdx >= ((int)(m_szTextBuffer.length())))return;
					unsigned char uFore;
					unsigned char uBack;
					iIdx = getUnicodeColorBytes(m_szTextBuffer,iIdx,&uFore,&uBack);
					iIdx--;
					if(uFore != KVI_NOCHANGE) m_iCurFore = uFore;
					else m_iCurFore = KVI_INPUT_DEF_FORE;
					if(uBack != KVI_NOCHANGE) m_iCurBack = uBack;
					else m_iCurBack = KVI_INPUT_DEF_BACK;
				}
				break;
				case 0:
					debug("KviInputEditor::Encountered invisible end of the string!");
					exit(0);
				break;
			}
		}
		iIdx++;
	}
}

void KviInputEditor::mousePressEvent(QMouseEvent * e)
{
	if(e->button() & Qt::LeftButton)
	{
		m_iCursorPosition = charIndexFromXPosition(e->pos().x());
		//move the cursor to
		int iAnchorX = xPositionFromCharIndex(m_iCursorPosition);
		if(iAnchorX > (width()-frameWidth())) m_iFirstVisibleChar++;
		m_iSelectionAnchorChar = m_iCursorPosition;
		selectOneChar(-1);
		//grabMouse(QCursor(crossCursor));
		repaintWithCursorOn();
		killDragTimer();
		m_iDragTimer = startTimer(KVI_INPUT_DRAG_TIMEOUT);

	} else if(e->button() & Qt::RightButton)
	{
		int iType = g_pActiveWindow->type();

		//Popup menu
		g_pInputPopup->clear();

		QString szClip;

		QClipboard * pClip = QApplication::clipboard();
		if(pClip)
		{
			szClip = pClip->text(QClipboard::Clipboard);

			int iOcc = szClip.count(QChar('\n'));

			if(!szClip.isEmpty())
			{
				if(szClip.length() > 60)
				{
					szClip.truncate(60);
					szClip.append("...");
				}
				szClip.replace(QChar('&'),"&amp;");
				szClip.replace(QChar('<'),"&lt;");
				szClip.replace(QChar('>'),"&gt;");
				szClip.replace(QChar('\n'),"<br>");

				QString szLabel = "<center><b>";
				szLabel += __tr2qs("Clipboard");
				szLabel += ":</b><br>";
				szLabel += szClip;
				szLabel += "<br><b>";

				QString szNum;
				szNum.setNum(iOcc);

				szLabel += szNum;
				szLabel += QChar(' ');
				szLabel += (iOcc == 1) ? __tr2qs("line break") : __tr2qs("line breaks");
				szLabel += "</b></center>";

				QLabel * pLabel = new QLabel(szLabel,g_pInputPopup);
				pLabel->setFrameStyle(QFrame::Raised | QFrame::StyledPanel);
				pLabel->setMargin(5);
				// FIXME: This does NOT work under Qt 4.x (they seem to consider it as bad UI design)

				delete pLabel;
			}
		}

		int iId = g_pInputPopup->insertItem(__tr2qs("Cu&t") + ACCEL_KEY(X),this,SLOT(cut()));
		g_pInputPopup->setItemEnabled(iId,hasSelection());
		iId = g_pInputPopup->insertItem(__tr2qs("&Copy") + ACCEL_KEY(C),this,SLOT(copyToClipboard()));
		g_pInputPopup->setItemEnabled(iId,hasSelection());
		iId = g_pInputPopup->insertItem(__tr2qs("&Paste") + ACCEL_KEY(V),this,SLOT(pasteClipboardWithConfirmation()));
		g_pInputPopup->setItemEnabled(iId,!szClip.isEmpty() && !m_bReadOnly);
		iId = g_pInputPopup->insertItem(__tr2qs("Paste (Slowly)"),this,SLOT(pasteSlow()));
		if ((iType == KVI_WINDOW_TYPE_CHANNEL) || (iType == KVI_WINDOW_TYPE_QUERY) || (iType == KVI_WINDOW_TYPE_DCCCHAT))
			g_pInputPopup->setItemEnabled(iId,!szClip.isEmpty() && !m_bReadOnly);
		else
			g_pInputPopup->setItemEnabled(iId,false);
		iId = g_pInputPopup->insertItem(__tr2qs("Paste &File") + ACCEL_KEY(L),this,SLOT(pasteFile()));
		if ((iType != KVI_WINDOW_TYPE_CHANNEL) && (iType != KVI_WINDOW_TYPE_QUERY) && (iType != KVI_WINDOW_TYPE_DCCCHAT))
			g_pInputPopup->setItemEnabled(iId,false);
		else
			g_pInputPopup->setItemEnabled(iId,!m_bReadOnly);
		if(m_bSpSlowFlag ==true)
		{
			iId = g_pInputPopup->insertItem(__tr2qs("Stop Paste"),this,SLOT(stopPasteSlow())); /*G&N 2005*/
		}
		iId = g_pInputPopup->insertItem(__tr2qs("Clear"),this,SLOT(clear()));
		g_pInputPopup->setItemEnabled(iId,!m_szTextBuffer.isEmpty() && !m_bReadOnly);
		g_pInputPopup->insertSeparator();
		iId = g_pInputPopup->insertItem(__tr2qs("Select All"),this,SLOT(selectAll()));
		g_pInputPopup->setItemEnabled(iId,(!m_szTextBuffer.isEmpty()));

		g_pInputPopup->insertSeparator();
		m_pIconMenu->clear();

		KviPointerHashTable<QString,KviTextIcon> * d = g_pTextIconManager->textIconDict();
		KviPointerHashTableIterator<QString,KviTextIcon> it(*d);
		QStringList szList;
		while(KviTextIcon * i = it.current())
		{
			szList.append(it.currentKey());
			++it;
		}
		szList.sort();

		KviTextIcon * pIcon;
		QPixmap * pPix;

		for(QStringList::Iterator iter = szList.begin(); iter != szList.end(); ++iter)
		{
			pIcon = g_pTextIconManager->lookupTextIcon(*iter);
			if(pIcon)
			{
				pPix = pIcon->pixmap();
				if(pPix) m_pIconMenu->insertItem(*pPix,*iter);
			}
		}

		g_pInputPopup->insertItem(*(g_pIconManager->getSmallIcon(KVI_SMALLICON_BIGGRIN)),__tr2qs("Insert Icon"),m_pIconMenu);
		g_pInputPopup->popup(mapToGlobal(e->pos()));
	} else {
		pasteSelectionWithConfirmation();
	}
}

void KviInputEditor::iconPopupActivated(int iId)
{
	if(!m_bReadOnly)
	{
		QString szText = m_pIconMenu->text(iId);
		if(!szText.isEmpty())
		{
			szText.prepend(KVI_TEXT_ICON);
			szText.append(' ');
			insertText(szText);
		}
	}
}

bool KviInputEditor::hasSelection()
{
	return ((m_iSelectionBegin != -1) && (m_iSelectionEnd != -1));
}

void KviInputEditor::copyToClipboard()
{
	if(!hasSelection()) return;
	QClipboard * pClip = QApplication::clipboard();
	if(!pClip) return;
	QString szTxt = m_szTextBuffer.mid(m_iSelectionBegin,(m_iSelectionEnd-m_iSelectionBegin)+1);
	pClip->setText(szTxt,QClipboard::Clipboard);
	repaintWithCursorOn();
}

void KviInputEditor::copyToSelection(bool bDonNotCopyToClipboard)
{
	if(!hasSelection()) return;
	QClipboard * pClip = QApplication::clipboard();
	if(!pClip) return;
	QString szTxt = m_szTextBuffer.mid(m_iSelectionBegin,(m_iSelectionEnd-m_iSelectionBegin)+1);
	if(pClip->supportsSelection())
		pClip->setText(szTxt,QClipboard::Selection);
	else if(!bDonNotCopyToClipboard)
		pClip->setText(szTxt,QClipboard::Clipboard);
	repaintWithCursorOn();
}

void KviInputEditor::moveCursorTo(int iIdx, bool bRepaint)
{
	if(iIdx < 0) iIdx = 0;
	if(iIdx > ((int)(m_szTextBuffer.length()))) iIdx = m_szTextBuffer.length();
	if(iIdx > m_iCursorPosition)
	{
		while(m_iCursorPosition < iIdx)
		{
			moveRightFirstVisibleCharToShowCursor();
			m_iCursorPosition++;
		}
	} else {
		m_iCursorPosition = iIdx;
		if(m_iFirstVisibleChar > m_iCursorPosition)m_iFirstVisibleChar = m_iCursorPosition;
	}
	if(bRepaint) repaintWithCursorOn();
}

void KviInputEditor::removeSelected()
{
	if(!hasSelection()) return;
	m_szTextBuffer.remove(m_iSelectionBegin,(m_iSelectionEnd-m_iSelectionBegin)+1);
	moveCursorTo(m_iSelectionBegin,false);
	selectOneChar(-1);
	repaintWithCursorOn();
}

void KviInputEditor::cut()
{
	if(!hasSelection()) return;
	QClipboard * pClip = QApplication::clipboard();
	if(!pClip) return;
	pClip->setText(m_szTextBuffer.mid(m_iSelectionBegin,(m_iSelectionEnd-m_iSelectionBegin)+1),QClipboard::Clipboard);
	m_szTextBuffer.remove(m_iSelectionBegin,(m_iSelectionEnd-m_iSelectionBegin)+1);
	moveCursorTo(m_iSelectionBegin,false);
	selectOneChar(-1);
	repaintWithCursorOn();
}

void KviInputEditor::insertText(const QString & szTxt)
{
	QString szText = szTxt; // crop away constness
	if(szText.isEmpty())return;

	//szText.replaceAll('\t'," "); //Do not paste tabs
	//szText.replace(QRegExp("\t")," "); // do not paste tabs

	m_bUpdatesEnabled = false;
	removeSelected();
	m_bUpdatesEnabled = true;

	if(szText.indexOf('\n') == -1)
	{
		m_szTextBuffer.insert(m_iCursorPosition,szText);
		m_szTextBuffer.truncate(m_iMaxBufferSize);
		moveCursorTo(m_iCursorPosition + szText.length());
	} else {
		//Multiline paste...do not execute commands here
		QString szBlock;
		while(!szText.isEmpty())
		{
			int iIdx = szText.indexOf('\n');
			if(iIdx != -1)
			{
				szBlock = szText.left(iIdx);
				//else szBlock = QChar(KVI_TEXT_RESET);
				szText.remove(0,iIdx+1);
			} else {
				szBlock = szText;
				szText  = "";
			}

			m_szTextBuffer.insert(m_iCursorPosition,szBlock);
			m_szTextBuffer.truncate(m_iMaxBufferSize);

			int iPos = 0;
			while((iPos < ((int)(m_szTextBuffer.length()))) && (m_szTextBuffer[iPos] < 33)) iPos++;
			if((iPos < ((int)(m_szTextBuffer.length()))) && (m_szTextBuffer[iPos] == QChar('/')))m_szTextBuffer.insert(iPos,"\\");

			returnPressed(iIdx != -1);
		}
	}
}

int KviInputEditor::replaceSegment(int iStart, int iLength, const QString & szText)
{
	m_szTextBuffer.remove(iStart, iLength);
	m_szTextBuffer.insert(iStart, szText);
	m_szTextBuffer.truncate(m_iMaxBufferSize);
	repaintWithCursorOn();

	int iInsertedLength = szText.length();
	int iMaxInsertedLength = m_iMaxBufferSize - iStart;
	if(iInsertedLength > iMaxInsertedLength) return iMaxInsertedLength;
	return iInsertedLength;
}

void KviInputEditor::pasteClipboardWithConfirmation()
{
	QClipboard * pClip = QApplication::clipboard();
	if(!pClip) return;
	QString szText = pClip->text(QClipboard::Clipboard);

	if(szText.contains(QChar('\n')) > 0)
	{
		if(m_pInputParent->inherits("KviInput"))
			((KviInput*)(m_pInputParent))->multiLinePaste(szText);
	} else {
		insertText(szText);
	}
}

void KviInputEditor::pasteSelectionWithConfirmation()
{
	QClipboard * pClip = QApplication::clipboard();
	if(!pClip) return;
	QString szText = pClip->text(pClip->supportsSelection() ? QClipboard::Selection : QClipboard::Clipboard);

	if(szText.contains(QChar('\n')) > 0)
	{
		if(m_pInputParent->inherits("KviInput"))
			((KviInput*)(m_pInputParent))->multiLinePaste(szText);
	} else {
		insertText(szText);
	}
}

void KviInputEditor::pasteSlow()
{
	KviKvsScript::run("spaste.clipboard",g_pActiveWindow);
	m_bSpSlowFlag = true;
}

void KviInputEditor::stopPasteSlow()
{
	KviKvsScript::run("spaste.stop",g_pActiveWindow);
	m_bSpSlowFlag = false;
}

void KviInputEditor::pasteFile()
{
	QString szTmp = QFileDialog::getOpenFileName(this,"Choose a file","","");
	if(szTmp != "")
	{
		szTmp.replace("\"", "\\\"");
		QString szTmp2 = QString("spaste.file \"%1\"").arg(szTmp);
		KviKvsScript::run(szTmp2,g_pActiveWindow);
		m_bSpSlowFlag = true;
	}
}

void KviInputEditor::selectAll()
{
	if(m_szTextBuffer.length() > 0)
	{
		m_iSelectionBegin = 0;
		m_iSelectionEnd = m_szTextBuffer.length()-1;
	}
	end();
}

void KviInputEditor::clear()
{
	m_szTextBuffer = "";
	selectOneChar(-1);
	home();
}

void KviInputEditor::setText(const QString szText)
{
	m_szTextBuffer = szText;
	m_szTextBuffer.truncate(m_iMaxBufferSize);
	selectOneChar(-1);
	end();
}

void KviInputEditor::mouseReleaseEvent(QMouseEvent * e)
{
	if(m_iDragTimer)
	{
		m_iSelectionAnchorChar =-1;
		//releaseMouse();
		killDragTimer();
	}
	if(hasSelection()) copyToSelection();
}

void KviInputEditor::killDragTimer()
{
	if(m_iDragTimer)
	{
		killTimer(m_iDragTimer);
		m_iDragTimer = 0;
	}
}

void KviInputEditor::timerEvent(QTimerEvent * e)
{
	if(e->timerId() == m_iCursorTimer)
	{
		if(!hasFocus() || !isVisible())
		{
			killTimer(m_iCursorTimer);
			m_iCursorTimer = 0;
			m_bCursorOn = false;
		} else m_bCursorOn = ! m_bCursorOn;
		update();
	} else {
		//Drag timer
		handleDragSelection();
	}
}

void KviInputEditor::handleDragSelection()
{
	if(m_iSelectionAnchorChar == -1) return;

	QPoint pnt = mapFromGlobal(QCursor::pos());

	if(pnt.x() <= 0)
	{
		//Left side dragging
		if(m_iFirstVisibleChar > 0) m_iFirstVisibleChar--;
		m_iCursorPosition = m_iFirstVisibleChar;
	} else if(pnt.x() >= width())
	{
		//Right side dragging...add a single character to the selection on the right
		if(m_iCursorPosition < ((int)(m_szTextBuffer.length())))
		{
			moveRightFirstVisibleCharToShowCursor();
			m_iCursorPosition++;
		} //else at the end of the selection...don't move anything
	} else {
		//Inside the window...
		m_iCursorPosition = charIndexFromXPosition(pnt.x());
	}

	if(m_iCursorPosition == m_iSelectionAnchorChar) selectOneChar(-1);
	else {
		if(m_iCursorPosition > m_iSelectionAnchorChar)
		{
			m_iSelectionBegin = m_iSelectionAnchorChar;
				m_iSelectionEnd   = m_iCursorPosition-1;
		} else {
			m_iSelectionBegin = m_iCursorPosition;
			m_iSelectionEnd   = m_iSelectionAnchorChar-1;
		}
	}
	repaintWithCursorOn();
}

void KviInputEditor::returnPressed(bool bRepaint)
{
	if (!m_szTextBuffer.isEmpty() /* && (!m_pHistory->current() || m_szTextBuffer.compare(*(m_pHistory->current())))*/)
	{
		if(m_pInputParent->inherits("KviInput"))
			KviInputHistory::instance()->add(new QString(m_szTextBuffer));

		m_pHistory->insert(0,new QString(m_szTextBuffer));
	}

	__range_valid(KVI_INPUT_MAX_LOCAL_HISTORY_ENTRIES > 1); //ABSOLUTELY NEEDED, if not, pHist will be destroyed...
	if(m_pHistory->count() > KVI_INPUT_MAX_LOCAL_HISTORY_ENTRIES)m_pHistory->removeLast();

	m_iCurHistoryIdx = -1;

	// FIXME: ALL THIS STUFF SHOULD BE CONVERTED TO QString
	/*
	if(m_pInputParent->inherits("KviInput"))
	{
		QString szBuffer(m_szTextBuffer);
		m_szTextBuffer="";
		selectOneChar(-1);
		m_iCursorPosition = 0;
		m_iFirstVisibleChar = 0;
		if(bRepaint)repaintWithCursorOn();
		KviUserInput::parse(szBuffer,m_pKviWindow);
	} else {
	*/
	emit enterPressed();
	/*
		return;
	}
	*/
}

void KviInputEditor::focusInEvent(QFocusEvent * e)
{
	if(m_iCursorTimer==0)
	{
		m_iCursorTimer = startTimer(KVI_INPUT_BLINK_TIME);
		m_bCursorOn = true;
		update();
	}
}

void KviInputEditor::focusOutEvent(QFocusEvent * e)
{
	if(m_iCursorTimer) killTimer(m_iCursorTimer);
	m_iCursorTimer = 0;
	m_bCursorOn = false;
	update();
}

void KviInputEditor::internalCursorRight(bool bShift)
{
	if(m_iCursorPosition >= ((int)(m_szTextBuffer.length()))) return;
	moveRightFirstVisibleCharToShowCursor();
	//Grow the selection if needed
	if(bShift)
	{
		if((m_iSelectionBegin > -1)&&(m_iSelectionEnd > -1))
		{
			if(m_iSelectionEnd == m_iCursorPosition-1)m_iSelectionEnd++;
			else if(m_iSelectionBegin == m_iCursorPosition)m_iSelectionBegin++;
			else selectOneChar(m_iCursorPosition);
		} else selectOneChar(m_iCursorPosition);
	} else selectOneChar(-1);
	m_iCursorPosition++;
}

void KviInputEditor::internalCursorLeft(bool bShift)
{
	if(m_iCursorPosition <= 0) return;

	if(bShift)
	{
		if((m_iSelectionBegin > -1) && (m_iSelectionEnd > -1))
		{
			if(m_iSelectionBegin == m_iCursorPosition)
				m_iSelectionBegin--;
			else if(m_iSelectionEnd == m_iCursorPosition-1)
				m_iSelectionEnd--;
			else selectOneChar(m_iCursorPosition - 1);
		} else selectOneChar(m_iCursorPosition - 1);
	} else selectOneChar(-1);

	m_iCursorPosition--;
	if(m_iFirstVisibleChar > m_iCursorPosition) m_iFirstVisibleChar--;
}

// remember the text before and after the cursor at this point, and put them
// before and after the text inserted by IM in InputMethodEndEvent.
//    hagabaka
void KviInputEditor::inputMethodStartEvent(QInputMethodEvent * e)
{
	removeSelected();
	m_iIMStart = m_iIMSelectionBegin = m_iCursorPosition;
	m_iIMLength = 0;
	m_bIMComposing = true;
	e->accept();
}

// Whenever the IM's preedit changes, update the visuals and internal data. refer to <http://doc.trolltech.com/3.3/QInputMethodEvent.html> */
//    hagabaka
void KviInputEditor::inputMethodComposeEvent(QInputMethodEvent * e)
{
	// replace the old pre-edit string with e->text()
	m_bUpdatesEnabled = false;

	m_iIMLength = replaceSegment(m_iIMStart, m_iIMLength, e->commitString());

	// update selection inside the pre-edit
	m_iIMSelectionBegin = m_iIMStart + e->replacementStart();
	m_iIMSelectionLength = e->replacementLength();
	moveCursorTo(m_iIMSelectionBegin);

	// repaint
	m_bUpdatesEnabled = true;
	repaintWithCursorOn();
	e->accept();
}

// Input method is done; put its resulting text to where the preedit area was
//    hagabaka
void KviInputEditor::inputMethodEndEvent(QInputMethodEvent * e)
{
	// replace the preedit area with the IM result text
	m_bUpdatesEnabled = false;

	m_iIMLength = replaceSegment(m_iIMStart, m_iIMLength, e->commitString());

	// move cursor to after the IM result text
	moveCursorTo(m_iIMStart + m_iIMLength);

	// repaint
	m_bUpdatesEnabled = true;
	repaintWithCursorOn();

	// reset data
	m_bIMComposing = false;
	e->accept();
}

// FIXME According to <http://www.kde.gr.jp/~asaki/how-to-support-input-method.html>, if the XIM
//  style used is OverTheTop, code needs to be added in keyPressEvent handler */
//    hagabaka
void KviInputEditor::keyPressEvent(QKeyEvent * e)
{
	// disable the keyPress handling when IM is in composition.
	if(m_bIMComposing)
	{
		e->ignore();
		return;
	}
	// completion thingies

	if(!m_bReadOnly)
	{
		if((e->key() == Qt::Key_Tab) || (e->key() == Qt::Key_Backtab))
		{
			completion(e->modifiers() & Qt::ShiftModifier);
			return;
		} else {
			m_bLastCompletionFinished=1;
		}
	}

	if(e->key() == Qt::Key_Escape)
	{
		emit escapePressed();
		return;
	}

	if((e->modifiers() & Qt::AltModifier) || (e->modifiers() & Qt::ControlModifier))
	{
		switch(e->key())
		{
			case Qt::Key_Backspace:
				if(m_pInputParent->inherits("KviInput"))
				{
					((KviInput*)(m_pInputParent))->multiLinePaste(m_szTextBuffer);
					clear();
					return;
				}
				break;
		}
	}


//Make CtrlKey and CommandKey ("Apple") behave equally on MacOSX.
//This way typical X11 and Apple shortcuts can be used simultanously within the input line.
#ifndef COMPILE_ON_MAC
	if(e->modifiers() & Qt::ControlModifier)
#else
	if((e->modifiers() & Qt::ControlModifier) || (e->modifiers() & Qt::MetaModifier))
#endif
	{
		switch(e->key())
		{
			case Qt::Key_Right:
				if(m_iCursorPosition < ((int)(m_szTextBuffer.length())))
				{
					// skip whitespace
					while(m_iCursorPosition < ((int)(m_szTextBuffer.length())))
					{
						if(!m_szTextBuffer.at(m_iCursorPosition).isSpace())break;
						internalCursorRight(e->modifiers() & Qt::ShiftModifier);
					}
					// skip nonwhitespace
					while(m_iCursorPosition < ((int)(m_szTextBuffer.length())))
					{
						if(m_szTextBuffer.at(m_iCursorPosition).isSpace())break;
						internalCursorRight(e->modifiers() & Qt::ShiftModifier);
					}
					repaintWithCursorOn();
				}
			break;
			case Qt::Key_Left:
				if(m_iCursorPosition > 0)
				{
					// skip whitespace
					while(m_iCursorPosition > 0)
					{
						if(!m_szTextBuffer.at(m_iCursorPosition - 1).isSpace())break;
						internalCursorLeft(e->modifiers() & Qt::ShiftModifier);
					}
					// skip nonwhitespace
					while(m_iCursorPosition > 0)
					{
						if(m_szTextBuffer.at(m_iCursorPosition - 1).isSpace())break;
						internalCursorLeft(e->modifiers() & Qt::ShiftModifier);
					}
					repaintWithCursorOn();
				}
			break;
			case Qt::Key_K:
			{
				if(!m_bReadOnly)
				{
					insertChar(KVI_TEXT_COLOR);
					int xPos = xPositionFromCharIndex(m_iCursorPosition);
					if(xPos > 24)xPos-=24;
					if(!g_pColorWindow)g_pColorWindow = new KviColorWindow();
					if(xPos+g_pColorWindow->width() > width())xPos = width()-(g_pColorWindow->width()+2);
					g_pColorWindow->move(mapToGlobal(QPoint(xPos,-35)));
					g_pColorWindow->popup(this);
				}
			}
			break;
			case Qt::Key_B:
				if(!m_bReadOnly) insertChar(KVI_TEXT_BOLD);
			break;
			case Qt::Key_O:
				if(!m_bReadOnly) insertChar(KVI_TEXT_RESET);
			break;
			case Qt::Key_U:
				if(!m_bReadOnly) insertChar(KVI_TEXT_UNDERLINE);
			break;
			case Qt::Key_R:
				if(!m_bReadOnly) insertChar(KVI_TEXT_REVERSE);
			break;
			case Qt::Key_P:
				if(!m_bReadOnly) insertChar(KVI_TEXT_CRYPTESCAPE); // DO NOT CRYPT THIS STUFF
			break;
			case Qt::Key_I:
			{
				if(!m_bReadOnly)
				{
					insertChar(KVI_TEXT_ICON); // THE NEXT WORD IS AN ICON NAME
					int iXPos = xPositionFromCharIndex(m_iCursorPosition);
					if(iXPos > 24)
						iXPos-=24;
					if(!g_pTextIconWindow)
						g_pTextIconWindow = new KviTextIconWindowWidget();

					if(iXPos+g_pTextIconWindow->width() > width())
						iXPos = width()-(g_pTextIconWindow->width()+2);
					g_pTextIconWindow->move(mapToGlobal(QPoint(iXPos,-KVI_TEXTICON_WIN_HEIGHT)));
					g_pTextIconWindow->popup(this);
				}
			}
			break;
			case Qt::Key_C:
				copyToClipboard();
			break;
			case Qt::Key_X:
				if(!m_bReadOnly) cut();
			break;
			case Qt::Key_V:
				if(!m_bReadOnly) pasteClipboardWithConfirmation();
			break;
			//case Qt::Key_Backspace:
			case Qt::Key_W:
				if(m_iCursorPosition > 0 && !m_bReadOnly && !hasSelection())
				{
					// skip whitespace
					while(m_iCursorPosition > 0)
					{
						if(!m_szTextBuffer.at(m_iCursorPosition - 1).isSpace())
							break;
						m_szTextBuffer.remove(m_iCursorPosition-1,1);
						m_iCursorPosition--;
						if(m_iFirstVisibleChar > m_iCursorPosition)
							m_iFirstVisibleChar--;
					}
					// skip nonwhitespace
					while(m_iCursorPosition > 0)
					{
						if(m_szTextBuffer.at(m_iCursorPosition - 1).isSpace())
							break;
						m_szTextBuffer.remove(m_iCursorPosition-1,1);
						m_iCursorPosition--;
						if(m_iFirstVisibleChar > m_iCursorPosition)
							m_iFirstVisibleChar--;
					}
					repaintWithCursorOn();
				}
			break;
			case Qt::Key_PageUp:
				if(!KVI_OPTION_BOOL(KviOption_boolEnableInputHistory))
					break;
				if(m_pInputParent->inherits("KviInput"))
					((KviInput*)(m_pInputParent))->historyButtonClicked();
			break;
			case Qt::Key_F:
				if(m_pKviWindow)
					if(m_pKviWindow->view())m_pKviWindow->view()->toggleToolWidget();
			break;
			case Qt::Key_A:
				m_iSelectionBegin=0;
				m_iSelectionEnd=m_szTextBuffer.length()-1;
				m_iCursorPosition=m_szTextBuffer.length();
				repaintWithCursorOn();
			break;
			case Qt::Key_Return:
			case Qt::Key_Enter:
				if(m_pInputParent->inherits("KviInput"))
				{
					QString szBuffer(m_szTextBuffer);
					m_szTextBuffer="";
					selectOneChar(-1);
					m_iCursorPosition = 0;
					m_iFirstVisibleChar = 0;
					repaintWithCursorOn();
					KviUserInput::parseNonCommand(szBuffer,m_pKviWindow);
					if (!szBuffer.isEmpty())
					{
						KviInputHistory::instance()->add(new QString(szBuffer));
						m_pHistory->insert(0,new QString(szBuffer));
					}

					__range_valid(KVI_INPUT_MAX_LOCAL_HISTORY_ENTRIES > 1); //ABSOLUTELY NEEDED, if not, pHist will be destroyed...
					if(m_pHistory->count() > KVI_INPUT_MAX_LOCAL_HISTORY_ENTRIES)m_pHistory->removeLast();

					m_iCurHistoryIdx = -1;
				}
				break;
			default:
				if(!m_bReadOnly) insertText(e->text());
			break;
		}
		return;
	}

	if((e->modifiers() & Qt::AltModifier) && (e->modifiers() & Qt::KeypadModifier))
	{
		// Qt::Key_Meta seems to substitute Qt::Key_Alt on some keyboards
		if((e->key() == Qt::Key_Alt) || (e->key() == Qt::Key_Meta))
		{
			m_szAltKeyCode = "";
			return;
		} else if((e->text().unicode()->toLatin1() >= '0') && (e->text().unicode()->toLatin1() <= '9'))
		{
			m_szAltKeyCode += e->text().unicode()->toLatin1();
			return;
		}

		//debug("%c",e->ascii());
		if(!m_bReadOnly) {
			insertText(e->text());
		}
		return;
	}

	if(e->modifiers() & Qt::ShiftModifier)
	{
		switch(e->key())
		{
			case Qt::Key_Insert:
				if(!m_bReadOnly) pasteClipboardWithConfirmation();
				return;
			break;
			case Qt::Key_PageUp:
				if(m_pKviWindow)
					if(m_pKviWindow->view())m_pKviWindow->view()->prevLine();
				return;
			break;
			case Qt::Key_PageDown:
				if(m_pKviWindow)
					if(m_pKviWindow->view())m_pKviWindow->view()->nextLine();
				return;
			break;
		}
	}

	switch(e->key())
	{
		case Qt::Key_Right:
			if(m_iCursorPosition < ((int)(m_szTextBuffer.length())))
			{
				internalCursorRight(e->modifiers() & Qt::ShiftModifier);
				repaintWithCursorOn();
			}
			break;
		case Qt::Key_Left:
			if(m_iCursorPosition > 0)
			{
				internalCursorLeft(e->modifiers() & Qt::ShiftModifier);
				repaintWithCursorOn();
			}
			break;
		case Qt::Key_Backspace:
			if(!m_bReadOnly)
			{
				if(hasSelection() && (m_iSelectionEnd >= m_iCursorPosition-1) && (m_iSelectionBegin <= m_iCursorPosition))
				{
					//remove the selection
					m_szTextBuffer.remove(m_iSelectionBegin,(m_iSelectionEnd-m_iSelectionBegin)+1);
					m_iCursorPosition = m_iSelectionBegin;
					if(m_iFirstVisibleChar > m_iCursorPosition)m_iFirstVisibleChar = m_iCursorPosition;
				} else if(m_iCursorPosition > 0) {
					m_iCursorPosition--;
					m_szTextBuffer.remove(m_iCursorPosition,1);
					if(m_iFirstVisibleChar > m_iCursorPosition)m_iFirstVisibleChar--;
				}
				selectOneChar(-1);
				repaintWithCursorOn();
			}
			break;
		case Qt::Key_Delete:
			if(!m_bReadOnly)
			{
				if(hasSelection()) removeSelected();
				else if(m_iCursorPosition < (int)m_szTextBuffer.length())
				{
					m_szTextBuffer.remove(m_iCursorPosition,1);
					selectOneChar(-1);
					repaintWithCursorOn();
				}
			}
			break;
		case Qt::Key_Home:
			if(m_iCursorPosition > 0)
			{
				if(e->modifiers() & Qt::ShiftModifier)
				{
					if((m_iSelectionBegin == -1)&&(m_iSelectionEnd == -1))m_iSelectionEnd = m_iCursorPosition - 1;
					m_iSelectionBegin = 0;
				} else {
					selectOneChar(-1);
				}
				home();
			}
			break;
		case Qt::Key_End://we should call it even the cursor is at the end for deselecting
			if(e->modifiers() & Qt::ShiftModifier)
			{
				if((m_iSelectionBegin == -1)&&(m_iSelectionEnd == -1))m_iSelectionBegin = m_iCursorPosition;
				m_iSelectionEnd = m_szTextBuffer.length()-1;
			} else {
				selectOneChar(-1);
			}
			end();
			break;
		case Qt::Key_Up:
			if(!m_bReadOnly)
			{
				if(m_pHistory->count() > 0)
				{
					if(m_iCurHistoryIdx < 0)
					{
						m_szSaveTextBuffer = m_szTextBuffer;
						m_szTextBuffer = *(m_pHistory->at(0));
						m_iCurHistoryIdx = 0;
					} else if(m_iCurHistoryIdx >= (int)(m_pHistory->count()-1))
					{
						m_szTextBuffer=m_szSaveTextBuffer;
						m_iCurHistoryIdx = -1;
					} else {
						m_iCurHistoryIdx++;
						m_szTextBuffer = *(m_pHistory->at(m_iCurHistoryIdx));
					}
					selectOneChar(-1);
					if(KVI_OPTION_BOOL(KviOption_boolInputHistoryCursorAtEnd))end();
					else home();
				}
			}
			break;
		case Qt::Key_Down:
			if(!m_bReadOnly)
			{
				if(m_pHistory->count() > 0)
				{
					if(m_iCurHistoryIdx < 0)
					{
						m_szSaveTextBuffer = m_szTextBuffer;
						m_szTextBuffer = *(m_pHistory->at(m_pHistory->count()-1));
						m_iCurHistoryIdx =m_pHistory->count()-1;
					} else if(m_iCurHistoryIdx == 0)
					{
						m_szTextBuffer=m_szSaveTextBuffer;
						m_iCurHistoryIdx = -1;
					} else {
						m_iCurHistoryIdx--;
						m_szTextBuffer = *(m_pHistory->at(m_iCurHistoryIdx));
					}
					selectOneChar(-1);
					if(KVI_OPTION_BOOL(KviOption_boolInputHistoryCursorAtEnd))end();
					else home();
				}
			}
			break;
		case Qt::Key_PageUp:
			if(m_pKviWindow)
				if(m_pKviWindow->view())m_pKviWindow->view()->prevPage();
		break;
		case Qt::Key_PageDown:
			if(m_pKviWindow)
				if(m_pKviWindow->view())m_pKviWindow->view()->nextPage();
		break;
		case Qt::Key_Return:
		case Qt::Key_Enter:
			returnPressed();
			break;
		case Qt::Key_Alt:
		case Qt::Key_Meta:
			m_szAltKeyCode = "";
			break;
		default:
			if(!e->text().isEmpty() && !m_bReadOnly)
				insertText(e->text());
		break;
	}
}

void KviInputEditor::keyReleaseEvent(QKeyEvent * e)
{
	if((e->key() == Qt::Key_Alt) || (e->key() == Qt::Key_Meta))
	{
		if(m_szAltKeyCode.hasData())
		{
			bool bOk;
			unsigned short uCh = m_szAltKeyCode.toUShort(&bOk);
			if(bOk && uCh != 0)
			{
				//debug("INSERTING CHAR %d",ch);
				insertChar(QChar(uCh));
				e->accept();
			}
		}
		m_szAltKeyCode = "";
	}
	e->ignore();
}

void KviInputEditor::getWordBeforeCursor(QString & szBuffer, bool * bIsFirstWordInLine)
{
	if(m_szTextBuffer.isEmpty() || m_iCursorPosition <= 0)
	{
		szBuffer = "";
		return;
	}

	szBuffer = m_szTextBuffer.left(m_iCursorPosition);

	int iIdx = szBuffer.lastIndexOf(' ');
	int iIdx2 = szBuffer.lastIndexOf(','); // This is for comma separated lists...
	int iIdx3 = szBuffer.lastIndexOf('(');
	int iIdx4 = szBuffer.lastIndexOf('"');
	if(iIdx2 > iIdx) iIdx = iIdx2;
	if(iIdx3 > iIdx) iIdx = iIdx3;
	if(iIdx4 > iIdx) iIdx = iIdx4;
	*bIsFirstWordInLine = false;
	if(iIdx > -1) szBuffer.remove(0,iIdx+1);
	else *bIsFirstWordInLine = true;
}

void KviInputEditor::completion(bool bShift)
{
	// FIXME: Spaces in directory completion can mess everything completely
	//        On windows the KVI_PATH_SEPARATOR_CHARacters are breaking everything...
	//        Well.... :D

	QString szWord;
	QString szMatch;
	bool bFirstWordInLine;

	getWordBeforeCursor(szWord,&bFirstWordInLine);
	if(szWord.isEmpty())
	{
		if(m_szLastCompletedNick.isEmpty()) return; // nothing to complete
		else {
			// this is standard nick completion continued
			standardNickCompletion(bShift,szWord,bFirstWordInLine);
			repaintWithCursorOn();
			return;
		}
	}
	KviPointerList<QString> tmp;
	tmp.setAutoDelete(true);

	bool bIsCommand = false;
	bool bIsDir = false;
	bool bIsNick = false;

	unsigned short uc = szWord[0].unicode();

	if(uc == '/')
	{
		if(bFirstWordInLine)
		{
			// command completion
			szWord.remove(0,1);
			if(szWord.isEmpty())return;
			KviKvsKernel::instance()->completeCommand(szWord,&tmp);
			bIsCommand = true;
		} else {
			// directory completion attempt
			g_pApp->completeDirectory(szWord,&tmp);
			bIsDir = true;
		}
	} else if(uc == '$')
	{
		// function/identifer completion
		szWord.remove(0,1);
		if(szWord.isEmpty()) return;
		KviKvsKernel::instance()->completeFunction(szWord,&tmp);
	} else if(uc == '#' || uc == '&' || uc == '!')
	{
		if(m_pKviWindow)
		{
			if( (szWord.length()==1) && (m_pKviWindow->windowName()[0].unicode()==uc))
			{
				szMatch=m_pKviWindow->windowName();
				szMatch.append(" ");
				replaceWordBeforeCursor(szWord,szMatch,false);
				repaintWithCursorOn();
				return;
			} else {
				if(m_pKviWindow->console())
					m_pKviWindow->console()->completeChannel(szWord,&tmp);
			}
		}

	//FIXME: Complete also on irc:// starting strings, not only irc.?
	} else if(KviQString::equalCIN(szWord,"irc.",4))
	{
		// irc server name
		if(m_pKviWindow)
			if(m_pKviWindow->console())
				m_pKviWindow->console()->completeServer(szWord,&tmp);
	} else {
		// empty word will end up here
		if(m_pUserListView)
		{
			if(KVI_OPTION_BOOL(KviOption_boolBashLikeNickCompletion))
			{
				m_pUserListView->completeNickBashLike(szWord,&tmp,bShift);
				bIsNick = true;
			} else {
				standardNickCompletion(bShift,szWord,bFirstWordInLine);
				repaintWithCursorOn();
				return;
			}
		}
	}

	// Lookup the longest exact match
	if(tmp.count() > 0)
	{
		if(tmp.count() == 1)
		{
			szMatch = *(tmp.first());
			if(bIsCommand)szMatch.append(' ');
			else if(bIsNick)
			{
				if(!KVI_OPTION_STRING(KviOption_stringNickCompletionPostfix).isEmpty())
				{
					if(bFirstWordInLine || (!KVI_OPTION_BOOL(KviOption_boolUseNickCompletionPostfixForFirstWordOnly)))
						szMatch.append(KVI_OPTION_STRING(KviOption_stringNickCompletionPostfix));
				}
			}
		} else {
			QString szAll;
			QString * szTmp = tmp.first();
			szMatch = *szTmp;
			int iWLen = szWord.length();
			for(; szTmp; szTmp = tmp.next())
			{
				if(szTmp->length() < szMatch.length())
					szMatch.remove(szTmp->length(),szMatch.length() - szTmp->length());
				// All the matches here have length >= word.len()!!!
				const QChar * b1 = KviQString::nullTerminatedArray(*szTmp) + iWLen;
				const QChar * b2 = KviQString::nullTerminatedArray(szMatch) + iWLen;
				const QChar * c1 = b1;
				const QChar * c2 = b2;
				if(bIsDir)while(c1->unicode() && (c1->unicode() == c2->unicode()))c1++,c2++;
				else while(c1->unicode() && (c1->toLower().unicode() == c2->toLower().unicode()))c1++,c2++;
				int iLen = iWLen + (c1 - b1);
				if(iLen < ((int)(szMatch.length())))szMatch.remove(iLen,szMatch.length() - iLen);
				if(!szAll.isEmpty())szAll.append(", ");
				szAll.append(*szTmp);
			}
			if(m_pKviWindow)
				m_pKviWindow->output(KVI_OUT_SYSTEMMESSAGE,__tr2qs("%d matches: %Q"),tmp.count(),&szAll);
		}
	} else
		if(m_pKviWindow)
			m_pKviWindow->outputNoFmt(KVI_OUT_SYSTEMMESSAGE,__tr2qs("No matches"));

	if(!szMatch.isEmpty())
	{
		//if(!bIsDir && !bIsNick)match = match.toLower(); <-- why? It is nice to have
		//						 $module.someFunctionName instad
		//						 of unreadable $module.somefunctionfame
		replaceWordBeforeCursor(szWord,szMatch,false);
	}

	repaintWithCursorOn();
}

void KviInputEditor::replaceWordBeforeCursor(const QString & szWord, const QString & szRreplacement, bool bRepaint)
{
	selectOneChar(-1);
	m_iCursorPosition -= szWord.length();
	m_szTextBuffer.remove(m_iCursorPosition,szWord.length());
	m_szTextBuffer.insert(m_iCursorPosition,szRreplacement);
	m_szTextBuffer.truncate(m_iMaxBufferSize);
	moveCursorTo(m_iCursorPosition + szRreplacement.length());
	if(bRepaint)repaintWithCursorOn();
}

void KviInputEditor::standardNickCompletion(bool bAddMask, QString & szWord, bool bFirstWordInLine)
{
	// FIXME: this could be really simplified...
	if(!m_pUserListView) return;
	selectOneChar(-1);

	QString szBuffer;
	if(m_szLastCompletedNick.isEmpty())
	{
		// New completion session: we NEED sth to complete
		if(szWord.isEmpty()) return;
		if(m_pUserListView->completeNickStandard(szWord,m_szLastCompletedNick,szBuffer,bAddMask))
		{
			// completed: save the buffer
			m_szLastCompletionBuffer          = m_szTextBuffer;
			m_iLastCompletionCursorPosition   = m_iCursorPosition;
			m_iLastCompletionCursorXPosition  = m_iLastCursorXPosition;
			m_iLastCompletionFirstVisibleChar = m_iFirstVisibleChar;
			m_szLastCompletedNick             = szBuffer;
			if(!KVI_OPTION_STRING(KviOption_stringNickCompletionPostfix).isEmpty())
			{
				if(bFirstWordInLine || (!KVI_OPTION_BOOL(KviOption_boolUseNickCompletionPostfixForFirstWordOnly)))
					szBuffer.append(KVI_OPTION_STRING(KviOption_stringNickCompletionPostfix));
			}
			replaceWordBeforeCursor(szWord,szBuffer,false);
			m_bLastCompletionFinished=0;
			// REPAINT CALLED FROM OUTSIDE!
		} // else no match at all
	} else  if(!m_bLastCompletionFinished) {
		// Old session
		// swap the buffers
		m_szTextBuffer                        = m_szLastCompletionBuffer;
		m_iCursorPosition                     = m_iLastCompletionCursorPosition;
		m_iLastCursorXPosition                = m_iLastCompletionCursorXPosition;
		m_iFirstVisibleChar                   = m_iLastCompletionFirstVisibleChar;
		// re-extract
		//word = m_szTextBuffer.left(m_iCursorPosition);

		getWordBeforeCursor(szWord,&bFirstWordInLine);
		if(szWord.isEmpty())return;
		if(m_pUserListView->completeNickStandard(szWord,m_szLastCompletedNick,szBuffer,bAddMask))
		{
			// completed
			m_szLastCompletedNick             = szBuffer;
			if(!KVI_OPTION_STRING(KviOption_stringNickCompletionPostfix).isEmpty())
			{
				if(bFirstWordInLine || (!KVI_OPTION_BOOL(KviOption_boolUseNickCompletionPostfixForFirstWordOnly)))
					szBuffer.append(KVI_OPTION_STRING(KviOption_stringNickCompletionPostfix));
			}
			replaceWordBeforeCursor(szWord,szBuffer,false);
			m_bLastCompletionFinished=0;
			// REPAINT CALLED FROM OUTSIDE!
		} else {
			m_bLastCompletionFinished=1;
			m_szLastCompletedNick = "";
		}
	} else {
		// Old session finished
		// re-extract
		//word = m_szTextBuffer.left(m_iCursorPosition);
		//getWordBeforeCursor(word,&bFirstWordInLine);
		if(szWord.isEmpty())return;
		if(m_pUserListView->completeNickStandard(szWord,"",szBuffer,bAddMask))
		{
			// completed
			m_szLastCompletionBuffer          = m_szTextBuffer;
			m_iLastCompletionCursorPosition   = m_iCursorPosition;
			m_iLastCompletionCursorXPosition  = m_iLastCursorXPosition;
			m_iLastCompletionFirstVisibleChar = m_iFirstVisibleChar;
			m_szLastCompletedNick             = szBuffer;
			if(!KVI_OPTION_STRING(KviOption_stringNickCompletionPostfix).isEmpty())
			{
				if(bFirstWordInLine || (!KVI_OPTION_BOOL(KviOption_boolUseNickCompletionPostfixForFirstWordOnly)))
					szBuffer.append(KVI_OPTION_STRING(KviOption_stringNickCompletionPostfix));
			}
			replaceWordBeforeCursor(szWord,szBuffer,false);
			m_bLastCompletionFinished=0;
			// REPAINT CALLED FROM OUTSIDE!
		} else {
			m_bLastCompletionFinished=1;
			m_szLastCompletedNick = "";
		}
	}
}

//Funky helpers
void KviInputEditor::end()
{
	m_iLastCursorXPosition = frameWidth();
	m_iCursorPosition = 0;
	m_iFirstVisibleChar = 0;
	while(m_iCursorPosition < ((int)(m_szTextBuffer.length())))
	{
		moveRightFirstVisibleCharToShowCursor();
		m_iCursorPosition++;
	}
	repaintWithCursorOn();
}

void KviInputEditor::home()
{
	m_iFirstVisibleChar = 0;
	m_iCursorPosition   = 0;
	repaintWithCursorOn();
}

void KviInputEditor::insertChar(QChar c)
{
	if(m_szTextBuffer.length() >= m_iMaxBufferSize)
		return;

	// Kill the selection
	if((m_iSelectionBegin > -1) || (m_iSelectionEnd > -1))
	{
		if((m_iCursorPosition >= m_iSelectionBegin) && (m_iCursorPosition <= m_iSelectionEnd))
		{
			m_bUpdatesEnabled = false;
			removeSelected();
			m_bUpdatesEnabled = true;
		}
	}
	selectOneChar(-1);
	m_szTextBuffer.insert(m_iCursorPosition,c);
	moveRightFirstVisibleCharToShowCursor();
	m_iCursorPosition++;
	repaintWithCursorOn();
}

void KviInputEditor::moveRightFirstVisibleCharToShowCursor()
{
	// :)
	QFontMetrics fm(KVI_OPTION_FONT(KviOption_fontInput));

	QChar c = m_szTextBuffer.at(m_iCursorPosition);

	m_iLastCursorXPosition += c.unicode() < 32 ? fm.width(getSubstituteChar(c.unicode())) + 3 : fm.width(c);;

	while(m_iLastCursorXPosition >= contentsRect().width()-2*KVI_INPUT_MARGIN)
	{
		c = m_szTextBuffer.at(m_iFirstVisibleChar);

		m_iLastCursorXPosition -= c.unicode() < 32 ? fm.width(getSubstituteChar(c.unicode())) + 3 : fm.width(c);;

		m_iFirstVisibleChar++;
	}
}

void KviInputEditor::repaintWithCursorOn()
{
	// :)
	if(m_bUpdatesEnabled)
	{
		m_bCursorOn = true;
		update();
	}
}

void KviInputEditor::selectOneChar(int iPos)
{
	m_iSelectionBegin = iPos;
	m_iSelectionEnd   = iPos;
}

int KviInputEditor::charIndexFromXPosition(int iXPos)
{
	int iCurXPos = frameWidth()+KVI_INPUT_MARGIN;
	int iCurChar = m_iFirstVisibleChar;
	int iBufLen  = m_szTextBuffer.length();

	QFontMetrics fm(KVI_OPTION_FONT(KviOption_fontInput));
	while(iCurChar < iBufLen)
	{
		QChar c = m_szTextBuffer.at(iCurChar);

		int iWidthCh = c.unicode() < 32 ? fm.width(getSubstituteChar(c.unicode())) + 3 : fm.width(c);

		if(iXPos < (iCurXPos+(iWidthCh/2))) return iCurChar;
		else if(iXPos < (iCurXPos+iWidthCh)) return (iCurChar+1);
		{
			iCurXPos += iWidthCh;
			iCurChar++;
		}
	}
	return iCurChar;
}

int  KviInputEditor::xPositionFromCharIndex(QFontMetrics & fm, int iChIdx, bool bContentsCoords)
{
	// FIXME: this could use fm.width(m_szTextBuffer,chIdx)
	int iCurXPos = bContentsCoords ? KVI_INPUT_MARGIN : frameWidth()+KVI_INPUT_MARGIN;
	int iCurChar = m_iFirstVisibleChar;
	while(iCurChar < iChIdx)
	{
		QChar c = m_szTextBuffer.at(iCurChar);

		iCurXPos += c.unicode() < 32 ? fm.width(getSubstituteChar(c.unicode())) + 3 : fm.width(c);;

		iCurChar++;
	}
	return iCurXPos;
}

int KviInputEditor::xPositionFromCharIndex(int iChIdx, bool bContentsCoords)
{
	// FIXME: this could use fm.width(m_szTextBuffer,chIdx)
	int iCurXPos = bContentsCoords ? KVI_INPUT_MARGIN : frameWidth()+KVI_INPUT_MARGIN;
	int iCurChar = m_iFirstVisibleChar;
	//debug("%i",g_pLastFontMetrics);
	if(!g_pLastFontMetrics) g_pLastFontMetrics = new QFontMetrics(KVI_OPTION_FONT(KviOption_fontInput));
	while(iCurChar < iChIdx)
	{
		QChar c = m_szTextBuffer.at(iCurChar);

		iCurXPos += c.unicode() < 32 ? g_pLastFontMetrics->width(getSubstituteChar(c.unicode())) + 3 : g_pLastFontMetrics->width(c);

		iCurChar++;
	}

	return iCurXPos;
}

/*
	@doc: texticons
	@type:
		generic
	@title:
		The KVIrc TextIcons extension
	@short:
		The KVIrc TextIcons extension
	@body:
		Starting from version 3.0.0 KVIrc supports the TextIcon extension
		to the standard IRC protocol. It is a mean for sending text enriched
		of small images without sending the images themselves.[br]
		The idea is quite simple: the IRC client (and it's user) associates
		some small images to text strings (called icon tokens) and the strings are sent
		in place of the images preceeded by a special escape character.[br]
		The choosen escape character is 29 (hex 0x1d) which corresponds
		to the ASCII group separator.[br]
		So for example if a client has the association of the icon token "rose" with a small
		icon containing a red rose flower then KVIrc could send the string
		"&lt;0x1d&gt;rose" in the message stream to ask the remote parties to
		display such an icon. If the remote parties don't have this association
		then they will simply strip the control code and display the string "rose",
		(eventually showing it in some enchanced way).[br]
		The icon tokens can't contain spaces
		so the receiving clients stop the extraction of the icon strings
		when a space, an icon escape or the message termination is encountered.
		[br]
		&lt;icon escape&gt; := character 0x1d (ASCII group separator)[br]
		&lt;icon token&gt; := any character with the exception of 0x1d, CR,LF and SPACE.[br]
		[br]
		Please note that this is a KVIrc extension and the remote clients
		that don't support this feature will not display the icon (and will
		eventually show the 0x1d character in the data stream).[br]
		If you like this feature please either convince the remote users
		to try KVIrc or tell them to write to their client developers asking
		for this simple feature to be implemented.[br]
*/


/*
	@doc: commandline
	@title:
		The Commandline Input Features
	@type:
		generic
	@short:
		Commandline input features
	@body:
		[big]Principles of operation[/big]
		[p]
		The idea is simple: anything that starts with a slash (/) character
		is interpreted as a command. Anything else is plain text that is
		sent to the target of the window (channel, query, dcc chat etc..).
		[/p]
		[big]The two operating modes[/big]
		[p]
		The commandline input has two operating modes: the "user friendly mode" and
		the "kvs mode". In the user friendly mode all the parameters of the commands
		are interpreted exactly like you type them. There is no special interpretation
		of $,%,-,( and ; characters. This allows you to type "/me is happy ;)", for example.
		In the kvs mode the full parameter interpretation is enabled and the commands
		work just like in any other script editor. This means that anything that
		starts with a $ is a function call, anything that starts with a % is a variable,
		the dash characters after command names are interpreted as switches and ; is the
		command separator. This in turn does NOT allow you to type "/me is happy ;)"
		because ; is the command separator and ) will be interpreted as the beginning
		of the next command. In KVS mode you obviously have to escape the ; character
		by typing "/me is happy \;)". The user friendly mode is good for everyday chatting
		and for novice users while the KVS mode is for experts that know that minimum about
		scripting languages. Please note that in the user-friendly mode you're not allowed
		to type multiple commands at once :).
		[/p]
		[big]Default Key Bindings:[/big][br]
		Ctrl+B: Inserts the 'bold' mIRC text control character<br>
		Ctrl+K: Inserts the 'color' mIRC text control character<br>
		Ctrl+R: Inserts the 'reverse' mIRC text control character<br>
		Ctrl+U: Inserts the 'underline' mIRC text control character<br>
		Ctrl+O: Inserts the 'reset' mIRC text control character<br>
		Ctrl+P: Inserts the 'non-crypt' (plain text) KVIrc control character used to disable encryption of the current text line<br>
		Ctrl+C: Copies the selected text to clipboard<br>
		Ctrl+X: Cuts the selected text<br>
		Ctrl+V: Pastes the clipboard contents (same as middle mouse click)<br>
		Ctrl+I: Inserts the 'icon' control code and pops up the icon list box<br>
		Ctrl+A: Select all<br>
		Ctrl+L: Paste file<br>
		CursorUp: Moves backward in the command history<br>
		CursorDown: Moves forward in the command history<br>
		CursorRight: Moves the cursor to the right<br>
		CursorLeft: Moves the cursor to the left :)<br>
		Shift+CursorLeft: Moves the selection to the left<br>
		Shift+RightCursor: Moves the selection to the right<br>
		Ctrl+CursorLeft: Moves the cursor one word left<br>
		Ctrl+CursorRight: Moves the cursor one word right<br>
		Ctrl+Shift+CursorLeft: Moves the selection one word left<br>
		Ctrl+Shift+CursorRight: Moves the selection one word right<br>
		Tab: Nickname, function/command, or filename completion (see below)<br>
		Shift+Tab: Hostmask or function/command completion (see below)<br>
		Alt+&lt;numeric_sequence&gt;: Inserts the character by ASCII/Unicode code<br>
		<example>
		Alt+32: Inserts ASCII/Unicode character 32: ' ' (a space)
		Alt+00032: Same as above :)
		Alt+13: Inserts the Carriage Return (CR) control character
		Alt+77: Inserts ASCII/Unicode character 77: 'M'
		Alt+23566: Inserts Unicode character 23566 (an ideogram)
		</example>
		Also look at the <a href="shortcuts.kvihelp">global shortcuts</a> reference.<br>
		If you drop a file on this widget, a <a href="parse.kvihelp">/PARSE &lt;filename&gt;</a> will be executed.<br>
		You can enable word substitution in the preferences dialog.<br>
		For example, if you choose to substitute "afaik" with "As far as I know",<br>
		when you will type "afaik" somewhere in the command line, and then
		press Space or Return, that word will be replaced with "As far as I know".<br>
		Experiment with it :)<br>
		The Tab key activates the completion of the current word.<br>
		If a word is prefixed with a '/', it is treated as a command to be completed,
		if it begins with '$', it is treated as a function or identifier to be completed,
		otherwise it is treated as a nickname or filename to be completed.<br>
		<example>
			/ec&lt;Tab&gt; will produce /echo&lt;space&gt
			/echo $loca&lt;Tab&gt; will produce /echo $localhost
		</example>
		Multiple matches are listed in the view window and the word is completed
		to the common part of all the matches.<br>
		<example>
			$sel&lt;Tab;&gt; will find multiple matches and produce $selected
		</example>
		Experiment with that too :)
*/
