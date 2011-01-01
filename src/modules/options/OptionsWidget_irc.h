#ifndef _OPTW_IRC_H_
#define _OPTW_IRC_H_
//=============================================================================
//
//   File : OptionsWidget_irc.h
//   Creation date : Sun Dec  2 18:59:49 2001 GMT by Szymon Stefanek
//
//   This file is part of the KVIrc irc client distribution
//   Copyright (C) 2001-2010 Szymon Stefanek (pragma at kvirc dot net)
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
//   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
//=============================================================================

#include "KviOptionsWidget.h"

#define KVI_OPTIONS_WIDGET_ICON_KviIrcOptionsWidget KVI_SMALLICON_IRC
#define KVI_OPTIONS_WIDGET_NAME_KviIrcOptionsWidget __tr2qs_no_lookup("IRC")
#define KVI_OPTIONS_WIDGET_PRIORITY_KviIrcOptionsWidget 80000
#define KVI_OPTIONS_WIDGET_CONTAINER_KviIrcOptionsWidget true

class OptionsWidget_irc : public KviOptionsWidget
{
	Q_OBJECT
public:
	OptionsWidget_irc(QWidget * parent);
	~OptionsWidget_irc();
};

#define KVI_OPTIONS_WIDGET_ICON_KviIrcGeneralOptionsWidget KVI_SMALLICON_IRC
#define KVI_OPTIONS_WIDGET_NAME_KviIrcGeneralOptionsWidget __tr2qs_no_lookup("General")
#define KVI_OPTIONS_WIDGET_PARENT_KviIrcGeneralOptionsWidget OptionsWidget_irc
#define KVI_OPTIONS_WIDGET_PRIORITY_KviIrcGeneralOptionsWidget 80000

class OptionsWidget_ircGeneral : public KviOptionsWidget
{
	Q_OBJECT
public:
	OptionsWidget_ircGeneral(QWidget * parent);
	~OptionsWidget_ircGeneral();
};


#define KVI_OPTIONS_WIDGET_ICON_KviIrcAdvancedOptionsWidget KVI_SMALLICON_IRC
#define KVI_OPTIONS_WIDGET_NAME_KviIrcAdvancedOptionsWidget __tr2qs_no_lookup("Advanced")
#define KVI_OPTIONS_WIDGET_PARENT_KviIrcAdvancedOptionsWidget OptionsWidget_irc
#define KVI_OPTIONS_WIDGET_PRIORITY_KviIrcAdvancedOptionsWidget 50000

class OptionsWidget_ircAdvanced : public KviOptionsWidget
{
	Q_OBJECT
public:
	OptionsWidget_ircAdvanced(QWidget * parent);
	~OptionsWidget_ircAdvanced();
};

#endif //_OPTW_IRC_H_
