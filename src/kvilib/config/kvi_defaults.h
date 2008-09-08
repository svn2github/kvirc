#ifndef _KVI_DEFAULTS_H_
#define _KVI_DEFAULTS_H_

//=============================================================================
//
//   File : kvi_defaults.h
//   Creation date : Wed Jun 21 2000 13:23:13 CEST by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2000-2004 Szymon Stefanek (pragma at kvirc dot net)
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

/**
* \file kvi_defaults.h
* \author Szymon Stefanek
* \brief This file contains (more or less) freely customizable compile time defaults
*
* You can safely play with the ones you understand the meaning of
*
* \def KVI_HOME_CONFIG_FILE_NAME Configuration file in home directory
* \def KVI_DEFAULT_INCOMING_SUBDIRECTORY_NAME Directory for incoming DCC files
* \def KVI_DEFAULT_NICKNAME1 Nickname
* \def KVI_DEFAULT_NICKNAME2 Alternative nickname 2
* \def KVI_DEFAULT_NICKNAME3 Alternative nickname 3
* \def KVI_DEFAULT_NICKNAME4 Alternative nickname 4
* \def KVI_DEFAULT_USERNAME Username
* \def KVI_DEFAULT_REALNAME Realname
* \def KVI_DEFAULT_PART_MESSAGE Part message
* \def KVI_DEFAULT_QUIT_MESSAGE Quit message
* \def KVI_DEFAULT_CTCP_USERINFO_REPLY Reply to CTCP USERINFO
* \def KVI_DEFAULT_CTCP_PAGE_REPLY Reply to CTCP PAGE
* \def KVI_DEFAULT_AWAY_MESSAGE Away message
* \def KVI_DEFAULT_AWAY_NICK Nickname when in away mode
* \def KVI_MAX_PENDING_AVATARS Maximum number of avatars to download simultanously
* \def KVI_MAX_RECENT_SERVERS Maximum number of servers to hold in "recent" menu
* \def KVI_MAX_RECENT_CHANNELS Maximum number of channels to hold in "recent" menu
* \def KVI_MAX_RECENT_NICKNAMES Maximum number of nicknames to hold in "recent" menu
* \def KVI_MAX_RECENT_TOPICS Maximum number of topics to hold in "recent" menu
*/

#include "kvi_settings.h"

#if defined(COMPILE_ON_WINDOWS) || defined(COMPILE_ON_MINGW) 
	#define KVI_HOME_CONFIG_FILE_NAME "kvirc4.ini"
#else
	#define KVI_HOME_CONFIG_FILE_NAME ".kvirc4.rc"
#endif

#define KVI_DEFAULT_INCOMING_SUBDIRECTORY_NAME "download"

#define KVI_DEFAULT_NICKNAME1 "newbie"
#define KVI_DEFAULT_NICKNAME2 "%nick%|2"
#define KVI_DEFAULT_NICKNAME3 "%nick%|3"
#define KVI_DEFAULT_NICKNAME4 "%nick%|4"
#define KVI_DEFAULT_USERNAME "kvirc"
#define KVI_DEFAULT_REALNAME "KVIrc " KVI_VERSION " '" KVI_RELEASE_NAME "' http://kvirc.net/"

#define KVI_DEFAULT_PART_MESSAGE "No matter how dark the night, somehow the Sun rises once again"
#define KVI_DEFAULT_QUIT_MESSAGE "KVIrc $version $version(r) http://www.kvirc.net/"
#define KVI_DEFAULT_CTCP_USERINFO_REPLY "I'm too lazy to edit this field."
#define KVI_DEFAULT_CTCP_PAGE_REPLY "Your message has been received and logged"
#define KVI_DEFAULT_AWAY_MESSAGE "I'm off to see the wizard."
#define KVI_DEFAULT_AWAY_NICK "%nick%|NotHere"

#define KVI_MAX_PENDING_AVATARS 20
#define KVI_MAX_RECENT_SERVERS 10
#define KVI_MAX_RECENT_CHANNELS 30
#define KVI_MAX_RECENT_NICKNAMES 10
#define KVI_MAX_RECENT_TOPICS 20

#endif //_KVI_DEFAULTS_H_
