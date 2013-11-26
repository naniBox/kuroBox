/*
	kuroBox / naniBox
	Copyright (c) 2013
	david morris-oliveros // dmo@nanibox.com // naniBox.com

	This file is part of kuroBox / naniBox.

	kuroBox / naniBox is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	kuroBox / naniBox is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


//-----------------------------------------------------------------------------
#ifndef UTILS_H
#define UTILS_H

//-----------------------------------------------------------------------------
#include <stdint.h>

//-----------------------------------------------------------------------------
uint16_t calc_checksum_16(const uint8_t * buf, uint32_t buf_size);

//-----------------------------------------------------------------------------
#ifdef WIN32
#define __PACKED__
#else
#define __PACKED__ __attribute__((packed))
#endif

//-----------------------------------------------------------------------------
#ifdef KBB_QT
#include <QtGlobal>
#define ASSERT(x) Q_ASSERT(x)
#else
#include <assert.h>
#define ASSERT(x) assert(x)
#endif

//-----------------------------------------------------------------------------
#ifdef __GNUC__OR_NOT__
#define STATIC_ASSERT_HELPER(expr, msg) \
	(!!sizeof(struct { unsigned int STATIC_ASSERTION__##msg: (expr) ? 1 : -1; }))
#define STATIC_ASSERT(expr, msg) \
	extern int (*assert_function__(void)) [STATIC_ASSERT_HELPER(expr, msg)]
#else
#define STATIC_ASSERT(expr, msg)   						\
	extern char STATIC_ASSERTION__##msg[1]; 			\
	extern char STATIC_ASSERTION__##msg[(expr)?1:2]
#endif /* #ifdef __GNUC__ */

#endif // UTILS_H
