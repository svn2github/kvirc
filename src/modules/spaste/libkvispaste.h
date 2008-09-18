#ifndef _KVISPASTE_H_
#define _KVISPASTE_H_
//=============================================================================
//
//   File : libkvispaste.h
//   Creation date : Thu Dec 27 2002 17:13:12 GMT by Juanjo �lvarez
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 2002 Juanjo �lvarez (juanjux@yahoo.es)
//   Copyright (C) 2002-2008 Szymon Stefanek (kvirc@tin.it)
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

#include "kvi_window.h"

typedef struct _SPasteThreadData {
	QString   * strData;
	KviWindow * win;
} SPasteThreadData;

#endif
