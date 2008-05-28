#ifndef _MP_WINAMPINTERFACE_H_
#define _MP_WINAMPINTERFACE_H_
//=============================================================================
//
//   File : mp_winampinterface.h
//   Created on Mon 28 Mar 2005 23:41:50 by Szymon Stefanek
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

#include "kvi_settings.h"

#if defined(COMPILE_ON_WINDOWS) || defined(COMPILE_ON_MINGW)

	#include "mp_interface.h"

	class KviWinampInterface : public KviMediaPlayerInterface
	{
	public:
		KviWinampInterface();
		~KviWinampInterface();
	public:
		virtual int detect(bool bStart);
		virtual bool prev();
		virtual bool next();
		virtual bool play();
		virtual bool stop();
		virtual bool pause();
		virtual bool quit();
		virtual QString nowPlaying();
		virtual QString mrl();
		virtual int position();
		virtual int length();
		virtual bool playMrl(const QString &mrl);
		virtual bool setVol(kvs_int_t &iVol);
		virtual int getVol();
		virtual bool jumpTo(kvs_int_t &iPos);
		virtual int sampleRate();
		virtual int bitRate();
		virtual int channels();
		virtual bool hide();
		virtual bool show();
		virtual bool minimize();
		virtual int getPlayListPos();
		virtual bool setPlayListPos(int &iPos);
		virtual int getListLength();
		virtual int getEqData(int &ival);
		virtual bool setEqData(int &iPos, int &iVal);
		virtual bool getRepeat();
		virtual bool getShuffle();
		virtual bool setRepeat(bool &bVal);
		virtual bool setShuffle(bool &bVal);
		virtual KviMediaPlayerInterface::PlayerStatus status();
	};
	
	MP_DECLARE_DESCRIPTOR(KviWinampInterface)
	
#endif //COMPILE_ON_WINDOWS

#endif //!_MP_WINAMPINTERFACE_H_
