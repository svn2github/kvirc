#ifndef _KVI_DEBUG_H_
#define _KVI_DEBUG_H_

//=============================================================================
//
//   File : kvi_debug.h
//   Creation date : Fri Mar 19 1999 03:10:39 CEST by Szymon Stefanek
//
//   This file is part of the KVirc irc client distribution
//   Copyright (C) 1999-2001 Szymon Stefanek (pragma at kvirc dot net)
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

#include <qglobal.h>

//=============================================================================
// This file contains the definition of the debug macros
// You can enable ALL the debugging output by uncommenting the next line
//
// #define _KVI_DEBUG_CHECK_RANGE_
//
//=============================================================================

#ifdef _KVI_DEBUG_CHECK_RANGE_
	#define __range_valid(_expr) if(!(_expr))debug("[kvirc]: ASSERT FAILED: \"%s\" is false in %s (%d)",#_expr,__FILE__,__LINE__)
	#define __range_invalid(_expr) if(_expr)debug("[kvirc]: ASSERT FAILED: \"%s\" is true in %s (%d)",#_expr,__FILE__,__LINE__)
#else
	#define __range_valid(_expr)
	#define __range_invalid(_expr)
#endif

#if defined(_KVI_DEBUG_) || defined(__KVI_DEBUG__)
	#define __ASSERT(_expr) if(!(_expr))debug("[kvirc]: ASSERT FAILED: \"%s\" is false in %s (%d)",#_expr,__FILE__,__LINE__)
#else
	#define __ASSERT(_expr)
#endif

#endif //_KVI_DEBUG_H_
