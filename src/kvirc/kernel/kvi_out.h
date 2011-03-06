#ifndef _KVI_OUT_H_
#define _KVI_OUT_H_
//=============================================================================
//
//   File : kvi_out.h
//   Creation date : Fri Jun 30 2000 14:13:21 by Szymon Stefanek
//
//   This file is part of the KVIrc irc client distribution
//   Copyright (C) 2000-2010 Szymon Stefanek (pragma at kvirc dot net)
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

#define KVI_OUT_NONE 0
#define KVI_OUT_SELECT 1
#define KVI_OUT_HIGHLIGHT 2
#define KVI_OUT_URL 3
#define KVI_OUT_LINK 4
#define KVI_OUT_PARSERERROR 5
#define KVI_OUT_PARSERWARNING 6
#define KVI_OUT_HOSTLOOKUP 7
#define KVI_OUT_SOCKETMESSAGE 8
#define KVI_OUT_SOCKETWARNING 9
#define KVI_OUT_SOCKETERROR 10
#define KVI_OUT_SYSTEMERROR 11
#define KVI_OUT_RAW 12
#define KVI_OUT_CONNECTION 13
#define KVI_OUT_SYSTEMWARNING 14
#define KVI_OUT_SYSTEMMESSAGE 15
#define KVI_OUT_UNHANDLED 16
#define KVI_OUT_SERVERINFO 17
#define KVI_OUT_MOTD 18
#define KVI_OUT_SERVERPING 19
#define KVI_OUT_JOIN 20
#define KVI_OUT_PART 21
#define KVI_OUT_UNRECOGNIZED 22
#define KVI_OUT_TOPIC 23
#define KVI_OUT_OWNPRIVMSG 24
#define KVI_OUT_CHANPRIVMSG 25
#define KVI_OUT_QUERYPRIVMSG 26
#define KVI_OUT_CTCPREPLY 27
#define KVI_OUT_CTCPREQUESTREPLIED 28
#define KVI_OUT_CTCPREQUESTIGNORED 29
#define KVI_OUT_CTCPREQUESTFLOOD 30
#define KVI_OUT_CTCPREQUESTUNKNOWN 31
#define KVI_OUT_ACTION 32
#define KVI_OUT_AVATAR 33
#define KVI_OUT_QUIT 34
#define KVI_OUT_SPLIT 35
#define KVI_OUT_QUITSPLIT 36
#define KVI_OUT_NICK 37
#define KVI_OUT_OP 38
#define KVI_OUT_DEOP 39
#define KVI_OUT_VOICE 40
#define KVI_OUT_DEVOICE 41
#define KVI_OUT_MODE 42
#define KVI_OUT_KEY 43
#define KVI_OUT_LIMIT 44
#define KVI_OUT_BAN 45
#define KVI_OUT_UNBAN 46
#define KVI_OUT_BANEXCEPT 47
#define KVI_OUT_BANUNEXCEPT 48
#define KVI_OUT_INVITEEXCEPT 49
#define KVI_OUT_INVITEUNEXCEPT 50
#define KVI_OUT_CHANMODE 51
#define KVI_OUT_WHO 52
#define KVI_OUT_DCCREQUEST 53
#define KVI_OUT_DCCMSG 54
#define KVI_OUT_DCCERROR 55
#define KVI_OUT_NICKNAMEPROBLEM 56
#define KVI_OUT_WHOISUSER 57
#define KVI_OUT_WHOISCHANNELS 58
#define KVI_OUT_WHOISIDLE 59
#define KVI_OUT_WHOISSERVER 60
#define KVI_OUT_WHOISOTHER 61
#define KVI_OUT_CREATIONTIME 62
#define KVI_OUT_NOTIFYONLINE 63
#define KVI_OUT_NOTIFYOFFLINE 64
#define KVI_OUT_OWNPRIVMSGCRYPTED 65
#define KVI_OUT_CHANPRIVMSGCRYPTED 66
#define KVI_OUT_QUERYPRIVMSGCRYPTED 67
#define KVI_OUT_DCCCHATMSG 68
#define KVI_OUT_DCCCHATMSGCRYPTED 69
#define KVI_OUT_IRC 70
#define KVI_OUT_KICK 71
#define KVI_OUT_LINKS 72
#define KVI_OUT_SPAM 73
#define KVI_OUT_ICQ 74
#define KVI_OUT_ICQMESSAGE 75
#define KVI_OUT_ICQMESSAGESENT 76
#define KVI_OUT_CHANNELNOTICE 77
#define KVI_OUT_CHANNELNOTICECRYPTED 78
#define KVI_OUT_QUERYNOTICE 79
#define KVI_OUT_QUERYNOTICECRYPTED 80
#define KVI_OUT_SERVERNOTICE 81
#define KVI_OUT_HALFOP 82
#define KVI_OUT_CTCPREPLYUNKNOWN 83
#define KVI_OUT_NICKSERV 84
#define KVI_OUT_CHANSERV 85
#define KVI_OUT_AWAY 86
#define KVI_OUT_IDENT 87
#define KVI_OUT_LIST 88
#define KVI_OUT_HALFDEOP 89
#define KVI_OUT_INVITE 90
#define KVI_OUT_MULTIMEDIA 91
#define KVI_OUT_QUERYTRACE 92
#define KVI_OUT_WALLOPS 93
#define KVI_OUT_JOINERROR 94
#define KVI_OUT_BROADCASTPRIVMSG 95
#define KVI_OUT_BROADCASTNOTICE 96
#define KVI_OUT_MEKICK 97
#define KVI_OUT_MEOP 98
#define KVI_OUT_MEVOICE 99
#define KVI_OUT_MEDEOP 100
#define KVI_OUT_MEDEVOICE 101
#define KVI_OUT_MEHALFOP 102
#define KVI_OUT_MEDEHALFOP 103
#define KVI_OUT_MEBAN 104
#define KVI_OUT_MEUNBAN 105
#define KVI_OUT_MEBANEXCEPT 106
#define KVI_OUT_MEBANUNEXCEPT 107
#define KVI_OUT_MEINVITEEXCEPT 108
#define KVI_OUT_MEINVITEUNEXCEPT 109
#define KVI_OUT_IGNORE 110
#define KVI_OUT_STATS 111
#define KVI_OUT_SSL 112
#define KVI_OUT_SEARCH 113
#define KVI_OUT_GENERICSUCCESS 114
#define KVI_OUT_GENERICSTATUS 115
#define KVI_OUT_GENERICVERBOSE 116
#define KVI_OUT_GENERICWARNING 117
#define KVI_OUT_GENERICERROR 118
#define KVI_OUT_GENERICCRITICAL 119
#define KVI_OUT_CHANADMIN 120
#define KVI_OUT_DECHANADMIN 121
#define KVI_OUT_MECHANADMIN 122
#define KVI_OUT_MEDECHANADMIN 123
#define KVI_OUT_USEROP 124
#define KVI_OUT_USERDEOP 125
#define KVI_OUT_MEUSEROP 126
#define KVI_OUT_MEDEUSEROP 127
#define KVI_OUT_VERBOSE 128
#define KVI_OUT_MECHANOWNER 129
#define KVI_OUT_CHANOWNER 130
#define KVI_OUT_MEDECHANOWNER 131
#define KVI_OUT_DECHANOWNER 132
#define KVI_OUT_SERVERERROR 133
#define KVI_OUT_HELP 134
#define KVI_OUT_TEXTENCODING 135
#define KVI_OUT_TORRENT 136
#define KVI_OUT_IRCOP 137
#define KVI_OUT_DEIRCOP 138
#define KVI_OUT_MEIRCOP 139
#define KVI_OUT_MEDEIRCOP 140
#define KVI_OUT_CAP 141
#define KVI_OUT_CHANURL 142
#define KVI_OUT_MEMOSERV 143
//#define KVI_NUM_MSGTYPE_OPTIONS 144
// UPDATE THE TOTAL COUNT IN KviOptions.h !!!!


#endif //_KVI_OPTIONS_H_
