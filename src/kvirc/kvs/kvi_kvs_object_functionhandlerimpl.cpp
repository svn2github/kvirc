//=============================================================================
//
//   File : kvi_kvs_object_functionhandlerimpl.cpp
//   Created on Sun 24 Apr 2005 04:01:14 by Szymon Stefanek
//
//   This file is part of the KVIrc IRC client distribution
//   Copyright (C) 2005 Szymon Stefanek <pragma at kvirc dot net>
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

#include "kvi_kvs_object_functionhandlerimpl.h"
#include "kvi_kvs_object_functioncall.h"
#include "kvi_kvs_script.h"



KviKvsObjectFunctionHandler * KviKvsObjectStandardTrueReturnFunctionHandler::clone()
{
	return new KviKvsObjectStandardTrueReturnFunctionHandler();
}

bool KviKvsObjectStandardTrueReturnFunctionHandler::call(KviKvsObject * pObject,KviKvsObjectFunctionCall * pCall)
{
	pCall->returnValue()->setBoolean(true);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

KviKvsObjectFunctionHandler * KviKvsObjectStandardFalseReturnFunctionHandler::clone()
{
	return new KviKvsObjectStandardFalseReturnFunctionHandler();
}

bool KviKvsObjectStandardFalseReturnFunctionHandler::call(KviKvsObject * pObject,KviKvsObjectFunctionCall * pCall)
{
	pCall->returnValue()->setBoolean(false);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

KviKvsObjectFunctionHandler * KviKvsObjectStandardNothingReturnFunctionHandler::clone()
{
	return new KviKvsObjectStandardNothingReturnFunctionHandler();
}

bool KviKvsObjectStandardNothingReturnFunctionHandler::call(KviKvsObject * pObject,KviKvsObjectFunctionCall * pCall)
{
	pCall->returnValue()->setNothing();
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

KviKvsObjectCoreCallFunctionHandler::KviKvsObjectCoreCallFunctionHandler(KviKvsObjectFunctionHandlerProc pProc,unsigned int uFlags)
: KviKvsObjectFunctionHandler(uFlags)
{
	m_pProc = pProc;
}

KviKvsObjectCoreCallFunctionHandler::KviKvsObjectCoreCallFunctionHandler(const KviKvsObjectCoreCallFunctionHandler &h)
: KviKvsObjectFunctionHandler(h.m_uFlags)
{
	m_pProc = h.m_pProc;
}

KviKvsObjectCoreCallFunctionHandler::~KviKvsObjectCoreCallFunctionHandler()
{
}

KviKvsObjectFunctionHandler * KviKvsObjectCoreCallFunctionHandler::clone()
{
	return new KviKvsObjectCoreCallFunctionHandler(*this);
}

bool KviKvsObjectCoreCallFunctionHandler::call(KviKvsObject * pObject,KviKvsObjectFunctionCall * pCall)
{
	// Actually m_pProc can't be 0: if it's zero then it's a BUG and should be traced down: we just crash
	//if(m_pProc)
	return (pObject->*m_pProc)(pCall);
	//else {
		// empty handler
		//pCall->returnValue()->setNothing();
		//return true;
	//}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

KviKvsObjectScriptFunctionHandler::KviKvsObjectScriptFunctionHandler(const QString &szScriptContext,const QString &szCode,unsigned int uFlags)
: KviKvsObjectFunctionHandler(uFlags)
{
	m_pHandler = new KviKvsScript(szScriptContext,szCode,KviKvsScript::InstructionList);
}

KviKvsObjectScriptFunctionHandler::KviKvsObjectScriptFunctionHandler(const KviKvsObjectScriptFunctionHandler &h)
: KviKvsObjectFunctionHandler(h.m_uFlags)
{
	// ref
	m_pHandler = new KviKvsScript(*(h.m_pHandler));
}

KviKvsObjectScriptFunctionHandler::~KviKvsObjectScriptFunctionHandler()
{
	delete m_pHandler;
}

bool KviKvsObjectScriptFunctionHandler::isScriptHandler()
{
	return true;
}

QString KviKvsObjectScriptFunctionHandler::scriptHandlerCode()
{
	return m_pHandler->code();
}




KviKvsObjectFunctionHandler * KviKvsObjectScriptFunctionHandler::clone()
{
	return new KviKvsObjectScriptFunctionHandler(*this);
}

bool KviKvsObjectScriptFunctionHandler::call(KviKvsObject * pObject,KviKvsObjectFunctionCall * pCall)
{
	KviKvsExtendedRunTimeData rd(pObject);
	return m_pHandler->run(pCall->window(),pCall->params(),pCall->returnValue(),KviKvsScript::PreserveParams,&rd);
}

