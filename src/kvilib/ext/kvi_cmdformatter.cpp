//=============================================================================
//
//   File : kvi_cmdformatter.cpp
//   Creation date : Tue Jun 12 2001 03:08:12 CEST by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2001-2007 Szymon Stefanek (pragma at kvirc dot net)
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

#define __KVILIB__

#include "kvi_cmdformatter.h"

namespace KviCommandFormatter
{
	bool hasLeadingChars(KviStr ** array,char c)
	{
		if(!(*array))return false; // can't have more leading chars
		bool bGotIt = false;
		while(*array)
		{
			if(*((*array)->ptr()) == c)
			{
				// found at least one such leading char
				bGotIt = true;
			} else {
				// we pretend this line to be empty
				KviStr tmp = *(*array);
				tmp.stripWhiteSpace();
				if(tmp.hasData())return false;
				*(*array) = ""; // set it to empty also in the main buffer
			}
			array++;
		}
		return bGotIt;
	}

	bool hasLeadingChars(QStringList &list,const QChar &c)
	{
		bool bGotIt = false;
		for(QStringList::Iterator it = list.begin();it != list.end();++it)
		{
			if((*it).length() < 1)continue;
			if((*it).at(0) == c)
			{
				// found at least one such leading char
				bGotIt = true;
			} else {
				// we pretend this line to be empty
				QString tmp = *it;
#ifdef COMPILE_USE_QT4
				tmp = tmp.trimmed();
#else
				tmp.stripWhiteSpace();
#endif
				if(!tmp.isEmpty())return false;
				*it = ""; // set it to empty also in the main buffer
			}
		}
		return bGotIt;
	}

	void trimLeading(KviStr ** array)
	{
		while(*array)
		{
			if((*array)->hasData())(*array)->cutLeft(1);
			array++;
		}
	}

	void trimLeading(QStringList &list)
	{
		for(QStringList::Iterator it = list.begin();it != list.end();++it)
		{
			(*it).remove(0,1);
		}
	}


	void addLeading(KviStr ** array,char c)
	{
		while(*array)
		{
			if((*array)->hasData())(*array)->prepend(c);
			array++;
		}
	}

	void addLeading(QStringList &list,const QChar & c)
	{
		for(QStringList::Iterator it = list.begin();it != list.end();++it)
		{
			(*it).prepend(c);
		}
	}


	void unindent(KviStr &buffer)
	{
		// we can format correctly up to 65536 lines (that's really enough)
		int realLen;
		KviStr ** array = buffer.splitToArray('\n',65536,&realLen);
		if(array)
		{
			while(hasLeadingChars(array,'\t') || hasLeadingChars(array,' '))trimLeading(array);
			buffer.joinFromArray(array,"\n",true);
			KviStr::freeArray(array);
		}
	}


	void unindent(QString &buffer)
	{
#ifdef COMPILE_USE_QT4
		QStringList list = buffer.split("\n",QString::KeepEmptyParts);
#else
		QStringList list = QStringList::split("\n",buffer,true);
#endif
		while(hasLeadingChars(list,QChar('\t')) || hasLeadingChars(list,QChar(' ')))trimLeading(list);
		//buffer = list.join("\n"); join implementation sux :D
		// we WANT the last newline
		buffer = "";
		for(QStringList::Iterator it = list.begin();it != list.end();++it)
		{
			buffer.append(*it);
			buffer.append(QChar('\n'));
		}
	}


	void bufferFromBlock(KviStr &buffer)
	{
		buffer.stripWhiteSpace();

		if((*(buffer.ptr()) == '{') && buffer.lastCharIs('}'))
		{
			// leading and trailing { must be stripped
			buffer.cutLeft(1);
			buffer.cutRight(1);
		}

		unindent(buffer);

		buffer.stripWhiteSpace();
	}

	void bufferFromBlock(QString &buffer)
	{
#ifdef COMPILE_USE_QT4
		buffer = buffer.trimmed();
#else
		buffer.stripWhiteSpace();
#endif

		if(buffer.isEmpty())return;

		if((buffer.at(0) == QChar('{')) && buffer.endsWith(QChar('}')))
		{
			buffer.remove(0,1);
			buffer.remove(buffer.length() - 1,1);
			while((buffer.at(0) == QChar('\n')) || (buffer.at(0) == QChar('\r')))
				buffer.remove(0,1);
		}

		unindent(buffer);

#ifdef COMPILE_USE_QT4
		buffer = buffer.trimmed();
#else
		buffer.stripWhiteSpace();
#endif
	}


	void indent(KviStr &buffer)
	{
		// we can format correctly up to 65536 lines (that's really enough)
		int realLen;
		KviStr ** array = buffer.splitToArray('\n',65536,&realLen);
		if(array)
		{
			addLeading(array,'\t');
			buffer.joinFromArray(array,"\n",true);
			KviStr::freeArray(array);
		}
	}

	void indent(QString &buffer)
	{
#ifdef COMPILE_USE_QT4
		QStringList list = buffer.split("\n",QString::KeepEmptyParts);
#else
		QStringList list = QStringList::split("\n",buffer,true);
#endif
		addLeading(list,QChar('\t'));
		//buffer = list.join("\n"); join implementation sux :D
		// we WANT the last newline
		buffer = "";
		for(QStringList::Iterator it = list.begin();it != list.end();++it)
		{
			buffer.append(*it);
			buffer.append(QChar('\n'));
		}
	}


	void blockFromBuffer(KviStr &buffer)
	{
		indent(buffer);
		buffer.prepend("{\n");
		buffer.stripRightWhiteSpace();
		buffer.ensureLastCharIs('\n');
		buffer.append("}\n");
	}

	void blockFromBuffer(QString &buffer)
	{
		indent(buffer);
		buffer.prepend("{\n");
		KviQString::stripRightWhiteSpace(buffer);
		KviQString::ensureLastCharIs(buffer,'\n');
		buffer.append("}\n");
	}
};

// FIXME: #warning "move popups and events to this formatting stuff!"
