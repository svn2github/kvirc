#ifndef _KVI_KVS_KERNEL_H_
#define _KVI_KVS_KERNEL_H_
//=============================================================================
//
//   File : kvi_kvs_kernel.h
//   Creation date : Tue 30 Sep 2003 13.46 CEST by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2003 Szymon Stefanek (pragma at kvirc dot net)
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

#include "kvi_settings.h"

#include "kvi_kvs_parser.h"
#include "kvi_list.h"
#include "kvi_qstring.h"

#include <qdict.h>

class KviKvsTreeNodeSpecialCommand;
class KviKvsObjectController;
class KviKvsAsyncOperationManager;
class KviKvsRunTimeContext;
class KviKvsVariantList;
class KviKvsSwitchList;
class KviKvsVariant;
class KviKvsScript;
class KviKvsHash;


typedef KviKvsTreeNodeCommand * (KviKvsParser::*specialCommandParsingRoutine)();
typedef struct _KviKvsSpecialCommandParsingRoutine
{
	specialCommandParsingRoutine proc;
} KviKvsSpecialCommandParsingRoutine;

typedef bool (*coreSimpleCommandExecRoutine)(KviKvsRunTimeContext * c,KviKvsVariantList * pParams,KviKvsSwitchList * pSwitches);
typedef struct _KviKvsCoreSimpleCommandExecRoutine
{
	coreSimpleCommandExecRoutine proc;
} KviKvsCoreSimpleCommandExecRoutine;

typedef bool (*coreFunctionExecRoutine)(KviKvsRunTimeContext * c,KviKvsVariantList * pParams,KviKvsVariant * pRetBuffer);
typedef struct _KviKvsCoreFunctionExecRoutine
{
	coreFunctionExecRoutine proc;
} KviKvsCoreFunctionExecRoutine;

typedef bool (*coreCallbackCommandExecRoutine)(KviKvsRunTimeContext * c,KviKvsVariantList * pParams,KviKvsSwitchList * pSwitches,const KviKvsScript * pCallback);
typedef struct _KviKvsCoreCallbackCommandExecRoutine
{
	coreCallbackCommandExecRoutine proc;
} KviKvsCoreCallbackCommandExecRoutine;


class KVIRC_API KviKvsKernel
{
public:
	KviKvsKernel();
	~KviKvsKernel();
private:
	static KviKvsKernel * m_pKvsKernel; // global kernel object

	QDict<KviKvsSpecialCommandParsingRoutine>     * m_pSpecialCommandParsingRoutineDict;
	
	QDict<KviKvsCoreSimpleCommandExecRoutine>     * m_pCoreSimpleCommandExecRoutineDict;
	QDict<KviKvsCoreCallbackCommandExecRoutine>   * m_pCoreCallbackCommandExecRoutineDict;
	QDict<KviKvsCoreFunctionExecRoutine>          * m_pCoreFunctionExecRoutineDict;
	
	KviKvsHash                                    * m_pGlobalVariables;
	KviKvsVariantList                             * m_pEmptyParameterList;
	
	KviKvsObjectController                        * m_pObjectController;
	KviKvsAsyncOperationManager                   * m_pAsyncOperationManager;
public:
	static void init();
	static void done();
	static KviKvsKernel * instance(){ return m_pKvsKernel; };
	
	KviKvsVariantList * emptyParameterList(){ return m_pEmptyParameterList; };

	KviKvsHash * globalVariables(){ return m_pGlobalVariables; };
	
	KviKvsObjectController * objectController(){ return m_pObjectController; };
	
	KviKvsAsyncOperationManager * asyncOperationManager(){ return m_pAsyncOperationManager; };

	void registerSpecialCommandParsingRoutine(const QString &szCmdName,KviKvsSpecialCommandParsingRoutine * r)
		{ m_pSpecialCommandParsingRoutineDict->replace(szCmdName,r); };
	KviKvsSpecialCommandParsingRoutine * findSpecialCommandParsingRoutine(const QString &szCmdName)
		{ return m_pSpecialCommandParsingRoutineDict->find(szCmdName); };

	void registerCoreSimpleCommandExecRoutine(const QString &szCmdName,KviKvsCoreSimpleCommandExecRoutine * r)
		{ m_pCoreSimpleCommandExecRoutineDict->replace(szCmdName,r); };
	KviKvsCoreSimpleCommandExecRoutine * findCoreSimpleCommandExecRoutine(const QString &szCmdName)
		{ return m_pCoreSimpleCommandExecRoutineDict->find(szCmdName); };

	void registerCoreFunctionExecRoutine(const QString &szFncName,KviKvsCoreFunctionExecRoutine * r)
		{ m_pCoreFunctionExecRoutineDict->replace(szFncName,r); };
	KviKvsCoreFunctionExecRoutine * findCoreFunctionExecRoutine(const QString &szFncName)
		{ return m_pCoreFunctionExecRoutineDict->find(szFncName); };

	void registerCoreCallbackCommandExecRoutine(const QString &szCmdName,KviKvsCoreCallbackCommandExecRoutine * r)
		{ m_pCoreCallbackCommandExecRoutineDict->replace(szCmdName,r); };
	KviKvsCoreCallbackCommandExecRoutine * findCoreCallbackCommandExecRoutine(const QString &szCmdName)
		{ return m_pCoreCallbackCommandExecRoutineDict->find(szCmdName); };

	void completeCommand(const QString &szCommandBegin,KviPtrList<QString> * pMatches);
	void completeFunction(const QString &szFunctionBegin,KviPtrList<QString> * pMatches);
	void completeModuleCommand(const QString &szModuleName,const QString &szCommandBegin,KviPtrList<QString> * matches);
	void completeModuleFunction(const QString &szModuleName,const QString &szFunctionBegin,KviPtrList<QString> * matches);
/*
	KviPtrList<QString> * completeCommandAllocateResult(const QString &szCommandBegin);
	KviPtrList<QString> * completeFunctionAllocateResult(const QString &szFunctionBegin);
	void freeCompletionResult(KviPtrList<QString> * l);
*/
};

#endif //!_KVI_KVS_KERNEL_H_
