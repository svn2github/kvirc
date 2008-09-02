#ifndef _KVI_KVS_CORESIMPLECOMMANDS_H_
#define _KVI_KVS_CORESIMPLECOMMANDS_H_
//=============================================================================
//
//   File : kvi_kvs_coresimplecommands.h
//   Created on Fri 31 Oct 2003 00:04:25 by Szymon Stefanek
//
//   This file is part of the KVIrc IRC client distribution
//   Copyright (C) 2003 Szymon Stefanek <pragma at kvirc dot net>
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

#include "kvi_frame.h"
#include "kvi_console.h"
#include "kvi_settings.h"
#include "kvi_kvs_runtimecontext.h"
#include "kvi_kvs_variantlist.h"
#include "kvi_kvs_switchlist.h"
#include "kvi_kvs_parameterprocessor.h"


#define KVSCSC(_name) bool _name(KviKvsRunTimeContext * __pContext,KviKvsVariantList * __pParams,KviKvsSwitchList * __pSwitches)

#define KVSCSC_pContext __pContext
#define KVSCSC_pParams __pParams
#define KVSCSC_pSwitches __pSwitches

#define KVSCSC_PARAMETER(a,b,c,d) KVS_PARAMETER(a,b,c,d)

#define KVSCSC_PARAMETERS_BEGIN \
	KVS_PARAMETERS_BEGIN(parameter_format_list)

#define KVSCSC_PARAMETERS_END \
	KVS_PARAMETERS_END \
	if(!KviKvsParameterProcessor::process(KVSCSC_pParams,KVSCSC_pContext,parameter_format_list))return false;

#define KVSCSC_REQUIRE_CONNECTION \
	if(!KVSCSC_pContext->window()->context())return KVSCSC_pContext->errorNoIrcContext(); \
	if(!KVSCSC_pContext->window()->connection())return KVSCSC_pContext->warningNoIrcConnection();

#define KVSCSC_pWindow __pContext->window()
#define KVSCSC_pConnection __pContext->window()->connection()
#define KVSCSC_pIrcContext __pContext->context()

namespace KviKvsCoreSimpleCommands
{
	// a_f
	KVSCSC(away);
	KVSCSC(back);
	KVSCSC(ban);
	KVSCSC(beep);
	KVSCSC(buttonctl);
	KVSCSC(ctcp);
	KVSCSC(debugCKEYWORDWORKAROUND);
	KVSCSC(deleteCKEYWORDWORKAROUND);
	KVSCSC(delpopupitem);
	KVSCSC(deop);
	KVSCSC(devoice);
	KVSCSC(echoprivmsg);
	KVSCSC(echo);
	KVSCSC(error);
	KVSCSC(eval);
	KVSCSC(eventctl);
	KVSCSC(exit);
	// g_l
	KVSCSC(halt);
	KVSCSC(help);
	KVSCSC(host);
	KVSCSC(inject);
	KVSCSC(join);
	KVSCSC(kick);
	KVSCSC(killtimer);
	KVSCSC(listtimers);
	// m_r
	KVSCSC(me);
	KVSCSC(mode);
	KVSCSC(nick);
	KVSCSC(notice);
	KVSCSC(op);
	KVSCSC(openurl);
	KVSCSC(option);
	KVSCSC(parse);
	KVSCSC(part);
	KVSCSC(play);
	KVSCSC(popup);
	KVSCSC(privmsg);
	KVSCSC(query);
	KVSCSC(quit);
	KVSCSC(raise);
	KVSCSC(raw);
	KVSCSC(returnCKEYWORDWORKAROUND);
	KVSCSC(rebind);
	KVSCSC(rfc2812wrapper);
	KVSCSC(run);
	// s_z
	KVSCSC(say);
	KVSCSC(server);
	KVSCSC(setmenu);
	KVSCSC(setreturn);
	KVSCSC(srand);
	KVSCSC(topic);
	KVSCSC(trload);
	KVSCSC(trunload);
	KVSCSC(unban);
	KVSCSC(voice);
	KVSCSC(warning);
	KVSCSC(who);
	KVSCSC(whois);
	KVSCSC(whowas);

	bool multipleModeCommand(KviKvsRunTimeContext * __pContext,KviKvsVariantList * __pParams,KviKvsSwitchList * __pSwitches,char plusminus,char flag);

	void init();
};

#endif //!_KVI_KVS_CORESIMPLECOMMANDS_H_
