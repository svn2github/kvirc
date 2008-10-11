//=============================================================================
//
//   File : class_memorybuffer.cpp
//   Creation date : Fri Mar 18 21:30:48 CEST 2005
//   by Tonino Imbesi(Grifisx) and Alessandro Carbone(Noldor)
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2005-2008 Alessandro Carbone (elfonol at gmail dot com)
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


#include "class_file.h"
#include "kvi_debug.h"
#include "kvi_locale.h"
#include "kvi_error.h"
#include "kvi_file.h"
#include "kvi_fileutils.h"
#include "class_memorybuffer.h"

#include <QStringList>
//#include <QFile>
#include <QTextStream>

// needed for $open()
const char * const mod_tbl[] =	{
					"Raw",
					"ReadOnly",
					"WriteOnly",
					"ReadWrite",
					"Append",
					"Truncate"
				  };

const QIODevice::OpenMode mod_cod[] = {
				QIODevice::Unbuffered,
				QIODevice::ReadOnly,
				QIODevice::WriteOnly,
				QIODevice::ReadWrite,
				QIODevice::Append,
				QIODevice::Truncate
			};

#define mod_num			(sizeof(mod_tbl) / sizeof(mod_tbl[0]))

/*
	@doc:	file
	@keyterms:
		file object class, creating files
	@title:
		file class
	@type:
		class
	@short:
		I/O device that operates on files
	@inherits:
		[class]object[/class]
	@description:
		This object class provides access to files - basic I/O
		operations, copying, removing etc...
	@functions:
		!fn: $setName(<file_name:string>)
		Sets the file name to <file_name>. It does [b]not move[/b] the file,
		it just changes the file the object is "pointing to". You can
		not change names of already open files.[br]
		See also: [classfnc]$open[/classfnc](),
		[classfnc]$name[/classfnc]().
		!fn: <string> $name()
		Returns name set by [classfnc]$setName[/classfnc]().[br]
		See also: [classfnc]$setName[/classfnc]().
		!fn: $open(<mode1:string>, <mode2:string>)
		Attempts to open the file in specified mode or modes "sum".
		Valid modes are:[br]
		[pre]
		Raw          - raw, non-buffered access[br]
		ReadOnly     - opens the file read-only[br]
		WriteOnly    - opens the file write-only[br]
		ReadWrite    - opens the file in read-write mode[br]
		Append       - opens the file in append mode. The file index is set to the end of the file.[br]
		Truncate     - truncates the file[br]
		[/pre]
		If you call this function without any parameters, the file is
		opened in ReadOnly mode.[br]
		When working with buffered files, data is not written directly
		to the file at once. You must call [classfnc]$flush[/classfnc]() to force it.[br]
		See also: [classfnc]$close[/classfnc](),
		[classfnc]$flush[/classfnc]().
		!fn: <boolean> $isOpen()
		Returns '1' if the file is open, '0' otherwise.
		!fn: $close()
		Closes the file, flushing the buffers first.[br]
		See also: [classfnc]$flush[/classfnc]().
		!fn: $flush()
		Flushes the file buffer to disk. Calling this after opening the
		file in 'Raw' mode doesn't make much sense.[br]
		See also: [classfnc]$open[/classfnc](),
		[classfnc]$close[/classfnc]().
		!fn: <integer> $size()
		Returns current file size.
		!fn: <boolean> $atEnd()
		Returns '1' if you have reached end of the file, '0' otherwise.
		[br]See also: [classfnc]$seek[/classfnc](),
		[classfnc]$where[/classfnc]().
		!fn: <integer> $where()
		Returns current position in the file (file index).[br]
		See also: [classfnc]$seek[/classfnc]().
		!fn: $seek(<index:integer>)
		Sets the file index to <index>.[br]
		See also: [classfnc]$where[/classfnc](),
		[classfnc]$atEnd[/classfnc]().
		!fn: $putch(<char>)
		Writes character <char> to the file and increments file
		index.[br]
		See also: [classfnc]$getch[/classfnc](),
		[classfnc]$ungetch[/classfnc]().
		!fn: <char> $getch()
		Reads a character from the file and increments file index.[br]
		See also: [classfnc]$putch[/classfnc](),
		[classfnc]$ungetch[/classfnc]().
		!fn: $ungetch(<char>)
		Puts the character <char> back to the file and
		decrements the file index. This is usually called to undo a
		$getch() call.[br]
		See also: [classfnc]$getch[/classfnc],
		[classfnc]$putch[/classfnc]().
		!fn: $readLine(<text_line:string>)
		Reads a line of text from the file and increments file index.
		!fn: $writeLine(<text_line:string>)
		Appends a line of text to the end of the file.
		$readBlock()
		$writeBlock()
		$writeHex()
		$readHex()
*/


KVSO_BEGIN_REGISTERCLASS(KviKvsObject_file,"file","object")
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"setName",setName)
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"name",name)

	KVSO_REGISTER_HANDLER(KviKvsObject_file,"open",open)
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"isOpen",isOpen)
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"close",close)
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"flush",flush)
	
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"size",size)

	KVSO_REGISTER_HANDLER(KviKvsObject_file,"atEnd",atEnd)
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"where",where)
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"seek",seek)
	
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"putch",putch)
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"getch",getch)
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"ungetch",unGetch)
	
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"readBlock",readBlock)
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"writeBlock", writeBlock)

	KVSO_REGISTER_HANDLER(KviKvsObject_file,"readHexBlock",readHexBlock)
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"writeHexBlock",writeHexBlock)

	KVSO_REGISTER_HANDLER(KviKvsObject_file,"readLine",readLine)
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"writeLine",writeLine)

	KVSO_REGISTER_HANDLER(KviKvsObject_file,"write",write)
	KVSO_REGISTER_HANDLER(KviKvsObject_file,"read",read)
	
	
KVSO_END_REGISTERCLASS(KviKvsObject_file)


KVSO_BEGIN_CONSTRUCTOR(KviKvsObject_file,KviKvsObject)

	m_pFile = new KviFile();

KVSO_END_CONSTRUCTOR(KviKvsObject_file)

KVSO_BEGIN_DESTRUCTOR(KviKvsObject_file)

	if (m_pFile) delete m_pFile;

KVSO_END_CONSTRUCTOR(KviKvsObject_file)
KVSO_CLASS_FUNCTION(file,setName)
{
	QString szName;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("file_name",KVS_PT_STRING,0,szName)
	KVSO_PARAMETERS_END(c)
	if (m_pFile) m_pFile->setFileName(szName);
	return true;
}

KVSO_CLASS_FUNCTION(file,name)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	c->returnValue()->setString(m_pFile->fileName());
	return true;
}

KVSO_CLASS_FUNCTION(file,open)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	QStringList modes;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("file_mode",KVS_PT_STRINGLIST,KVS_PF_OPTIONAL,modes)
	KVSO_PARAMETERS_END(c)
	
	if(m_pFile->fileName().isEmpty())
	{
		c->warning(__tr2qs("Empty filename string"));
		return true;
	}
	QIODevice::OpenMode mod,sum;
	if (modes.empty()) sum = QIODevice::ReadWrite|QIODevice::Append; // if no parameters given, default to ReadWrite | Append
	else
	{
		for ( int idx=0;idx<modes.count();idx++)
		{
			mod = QIODevice::ReadOnly;
			for(unsigned int j = 0; j < mod_num; j++)
			{
				if(KviQString::equalCI(modes.at(idx), mod_tbl[j]))
				{
					mod=mod_cod[j];
					break;
				}
			}
			if(mod!=QIODevice::ReadOnly)
				sum = sum | mod;
			else
				c->warning(__tr2qs("No such open mode: '%Q'"),&modes.at(idx));
		}
	}

	m_pFile->open(sum);
	return true;
}

KVSO_CLASS_FUNCTION(file,isOpen)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	c->returnValue()->setBoolean(m_pFile->isOpen());
	return true;
}

KVSO_CLASS_FUNCTION(file,close)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	if (!m_pFile->isOpen()) c->warning(__tr2qs("File is not open!"));
	else m_pFile->close();
	return true;
}
KVSO_CLASS_FUNCTION(file,flush)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	if (!m_pFile->isOpen()) c->warning(__tr2qs("File is not open!"));
	else m_pFile->flush();
	return true;
}

KVSO_CLASS_FUNCTION(file,size)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	c->returnValue()->setInteger((kvs_int_t)(m_pFile->size()));
	return true;
}
KVSO_CLASS_FUNCTION(file,atEnd)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	c->returnValue()->setInteger((kvs_int_t)(m_pFile->atEnd()));
	return true;
}


KVSO_CLASS_FUNCTION(file,where)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	if (!m_pFile->isOpen()) c->warning(__tr2qs("File is not open!"));
	else c->returnValue()->setInteger((kvs_int_t)(m_pFile->pos()));
	return true;
}
KVSO_CLASS_FUNCTION(file,seek)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	kvs_uint_t uIndex;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("index",KVS_PT_UNSIGNEDINTEGER,0,uIndex)
	KVSO_PARAMETERS_END(c)
	if(!m_pFile->isOpen())
		c->warning(__tr2qs("File is not open !"));
	else m_pFile->seek(uIndex);
	return true;
}

KVSO_CLASS_FUNCTION(file,putch)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	QString szChar;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("char",KVS_PT_STRING,0,szChar)
	KVSO_PARAMETERS_END(c)
	if (szChar.length()>1)c->warning(__tr2qs("Argument to long, using only first char"));
	const char *ch=szChar.toUtf8().data();
	if (!m_pFile->putChar(ch[0])) c->warning(__tr2qs("Write error occured !"));
	return true;
}
KVSO_CLASS_FUNCTION(file,getch)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	if(!m_pFile->isOpen())
		c->warning(__tr2qs("File is not open !"));
	else
	{
		char ch;
		if (!m_pFile->getChar(&ch)) c->warning(__tr2qs("Read error occured !"));	// c->error ?
		QString szChar = QChar(ch);
		c->returnValue()->setString(szChar);
	}
	return true;
}
KVSO_CLASS_FUNCTION(file,unGetch)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	QString szChar;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("char",KVS_PT_STRING,0,szChar)
	KVSO_PARAMETERS_END(c)
	if(!m_pFile->isOpen())
		c->warning(__tr2qs("File is not open !"));
	else
	{
		if (szChar.length()>1) c->warning(__tr2qs("Argument to long, using only first char"));
		const char *ch=szChar.toUtf8().data();
		m_pFile->ungetChar(ch[0]);
	}
	return true;
}
KVSO_CLASS_FUNCTION(file,readBlock)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	kvs_uint_t uLen;
	KviKvsObject * pObject;
	kvs_hobject_t hObject;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("length",KVS_PT_UNSIGNEDINTEGER,0,uLen)
		KVSO_PARAMETER("hobject",KVS_PT_HOBJECT,KVS_PF_OPTIONAL,hObject)
	KVSO_PARAMETERS_END(c)
	if(!m_pFile->isOpen()){
		c->warning(__tr2qs("File is not open !"));
	}
	else
	{
		if (uLen>(kvs_uint_t)m_pFile->size()) uLen=m_pFile->size();
		if (hObject)
		{
			pObject=KviKvsKernel::instance()->objectController()->lookupObject(hObject);
			if (!pObject)
			{
				c->warning(__tr2qs("Buffer parameter is not an object"));
				return true;
			}
			if (!pObject->inherits("KviKvsObject_memorybuffer"))
			{
				debug ("Classname %s",pObject->metaObject()->className());
				c->warning(__tr2qs("Buffer parameter is not a memorybuffer object"));
				return true;
			}
			//m_pFile->flush();
			((KviKvsObject_memorybuffer *)pObject)->pBuffer()->append(m_pFile->read(uLen));
			return true;
		}
		else
		{
			char * buff = new char[uLen + 1];
			//m_pFile->flush(); // advice from QFile man page (to avoid trash)
			int rlen = m_pFile->read(buff, uLen);
			buff[rlen] = '\0';
			QString szBlock(buff);
			c->returnValue()->setString(szBlock);
		}
	}
	return true;
}
KVSO_CLASS_FUNCTION(file,read)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	QString szType;
	KVSO_PARAMETERS_BEGIN(c)
			KVSO_PARAMETER("type",KVS_PT_STRING,0,szType)
	KVSO_PARAMETERS_END(c)
	if(!m_pFile->isOpen()){
		c->warning(__tr2qs("File is not open !"));
	}
	else
	{
		if(KviQString::equalCI(szType, "String"))
		{
			QString szStr;
			m_pFile->load(szStr);
			c->returnValue()->setString(szStr);
		}
		else if(KviQString::equalCI(szType, "Integer"))
		{
			int iValue;
			m_pFile->load(iValue);
			c->returnValue()->setInteger(iValue);
		}
		if(KviQString::equalCI(szType, "Array"))
		{
			QString szData;
			m_pFile->load(szData);
			KviKvsVariant *pVar = KviKvsVariant::unserialize(szData);
			if (pVar->isArray())
				c->returnValue()->setArray(pVar->array());
			else
				c->warning(__tr2qs("The incoming data doesn't an array"));
		}
		if(KviQString::equalCI(szType, "Dict"))
		{
			QString szData;
			m_pFile->load(szData);
			KviKvsVariant *pVar = KviKvsVariant::unserialize(szData);
			if (pVar->isHash())
				c->returnValue()->setHash(pVar->hash());
			else
				c->warning(__tr2qs("The incoming data doesn't a Dictionary"));
		}
		if(KviQString::equalCI(szType, "String"))
		{
			QString szStr;
			m_pFile->load(szStr);
			c->returnValue()->setString(szStr);
		}
	}
	return true;
}
KVSO_CLASS_FUNCTION(file,write)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	KviKvsVariant * pVariantData;
	KVSO_PARAMETERS_BEGIN(c)
			KVSO_PARAMETER("variant_data",KVS_PT_VARIANT,0,pVariantData)
	KVSO_PARAMETERS_END(c)
	if(!m_pFile->isOpen())c->warning(__tr2qs("File is not open !"));
	else
	{
		if (pVariantData->isInteger()) 
		{
			kvs_int_t num;
			pVariantData->asInteger(num);
			m_pFile->save(num);
		}
		if (pVariantData->isHash() || pVariantData->isArray() || pVariantData->isString())
		{
			QString szTmp;
			pVariantData->serialize(szTmp);
			m_pFile->save(szTmp);
		}

	}
	return true;
}


KVSO_CLASS_FUNCTION(file,writeBlock)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	kvs_uint_t uLen;
	KviKvsObject * pObject;
	KviKvsVariant * pVariantData;
	kvs_hobject_t hObject;
	KVSO_PARAMETERS_BEGIN(c)
			KVSO_PARAMETER("string_or_hobject",KVS_PT_VARIANT,0,pVariantData)
			KVSO_PARAMETER("length",KVS_PT_UNSIGNEDINTEGER,KVS_PF_OPTIONAL,uLen)
	KVSO_PARAMETERS_END(c)
	if(!m_pFile->isOpen())c->warning(__tr2qs("File is not open !"));
	else
	{
		if (pVariantData->isHObject())
		{
			pVariantData->asHObject(hObject);
			pObject=KviKvsKernel::instance()->objectController()->lookupObject(hObject);
			if (!pObject)
			{
				c->warning(__tr2qs("Buffer parameter is not an object"));
				return true;
			}
			if (pObject->inherits("KviKvsObject_memorybuffer"))
			{
				if (!uLen)
				{
					if (((KviKvsObject_memorybuffer *)pObject)->pBuffer()->size())
						uLen=((KviKvsObject_memorybuffer *)pObject)->pBuffer()->size();
					else
					{
						c->warning(__tr2qs("The memoryBuffer object is empty: nothing will be saved"));
					}
				}
				const char *pData=((KviKvsObject_memorybuffer *)pObject)->pBuffer()->data();
				int rlen=m_pFile->write(pData,uLen);
				c->returnValue()->setInteger(rlen);
				//m_pFile->flush();
			}
			else
			{
				c->warning(__tr2qs("Buffer parameter is not a memorybufferobject"));
				return true;
			}
		
		}
		else
		{
			if (!pVariantData->isString()){
				c->warning(__tr2qs("Block must been a string or a memorybufferobject"));
				return true;
			}
			QString szBlock;
			pVariantData->asString(szBlock);
			const char *block=szBlock.toUtf8().data();
			int rlen = m_pFile->write(block, uLen);
			c->returnValue()->setInteger(rlen);
			//m_pFile->flush();
		}
	}
	return true;
}
KVSO_CLASS_FUNCTION(file,readLine)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	if(!m_pFile->isOpen())
		c->warning(__tr2qs("File is not open !"));
	else
	{
		QString buffer;
		KviFileUtils::readLine(m_pFile,buffer);
		c->returnValue()->setString(buffer);
	}
	return true;
}
KVSO_CLASS_FUNCTION(file,writeLine)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	QString szLine;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("text_line",KVS_PT_STRING,0,szLine)
	KVSO_PARAMETERS_END(c)
	
	if(!m_pFile->isOpen())
		c->warning(__tr2qs("File is not open !"));
	else
	{
		QTextStream ts(m_pFile);
		ts << szLine;
	}
	return true;
}
KVSO_CLASS_FUNCTION(file,readHexBlock)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	kvs_uint_t uLen;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("lenght",KVS_PT_UNSIGNEDINTEGER,0,uLen)
	KVSO_PARAMETERS_END(c)
	
	if(!m_pFile->isOpen())
		c->warning(__tr2qs("File is not open !"));
	else
	{
		if (uLen>(kvs_uint_t)m_pFile->size()) uLen=m_pFile->size();
		char * buff = new char[uLen];
		char * str = new char [(uLen*2)+1];
		m_pFile->flush(); // advice from QFile man page (to avoid trash)
		int rlen = m_pFile->read(buff, uLen);
		int index=0;
		unsigned char byte,msb,lsb=0;
		for (int i=0;i<rlen;i++)
		{
			byte=(unsigned char)buff[i];
			lsb=byte & 0x0f;
			msb=byte>>4;
			
			msb>9?msb+='7':msb+='0'; 
			lsb>9?lsb+='7':lsb+='0'; 
			str[index++]=msb; 
			str[index++]=lsb;
		}		
		str[index]='\0';
		c->returnValue()->setString(str);
		delete str;
		delete buff;
	}
	return true;
}
KVSO_CLASS_FUNCTION(file,writeHexBlock)
{
	CHECK_INTERNAL_QPOINTER(m_pFile)
	QString szBlock;
	kvs_uint_t uLen;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("text_block",KVS_PT_STRING,0,szBlock)
		KVSO_PARAMETER("lenght",KVS_PT_UNSIGNEDINTEGER,KVS_PF_OPTIONAL,uLen)
	KVSO_PARAMETERS_END(c)
	
	if(!m_pFile->isOpen()){
		c->warning(__tr2qs("File is not open !"));
		return true;
	}
	if (szBlock.length()%2) 
	{
		c->warning(__tr2qs("Lenght of hex string is not multiple of 2"));
		return true;
	}
	if (uLen>(szBlock.length()/2)|| !uLen) uLen=szBlock.length();
	else uLen*=2;
	unsigned char byte,lsb,msb;
	for(int i=0;i<uLen;i+=2)
	{
		msb=szBlock.at(i).toAscii();
		lsb=szBlock.at(i+1).toAscii();
		
		if (((msb>='A' && msb<='F')||(msb>='0' && msb<='9')) && ((lsb>='A' && lsb<='F')|| (lsb>='0' && lsb<='9')))
		{
					msb>='A'?msb-='7':msb-='0';
					lsb>='A'?lsb-='7':lsb-='0';
		}
		else{
			c->warning("The hex string is not correct!");
			return true;
		}
		byte=(msb*16)+lsb;
		m_pFile->putChar(byte);
	}
	c->returnValue()->setInteger(uLen/2);
	return true;
}

#ifndef COMPILE_USE_STANDALONE_MOC_SOURCES
#include "m_class_file.moc"
#endif //!COMPILE_USE_STANDALONE_MOC_SOURCES

