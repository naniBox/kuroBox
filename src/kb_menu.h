/*
	kuroBox / naniBox
	Copyright (c) 2013
	david morris-oliveros // naniBox.com

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
#ifndef _naniBox_kuroBox_menu
#define _naniBox_kuroBox_menu

//-----------------------------------------------------------------------------
#include "kb_util.h"

//-----------------------------------------------------------------------------
int kuroBoxMenuInit(void);
int kuroBoxMenuStop(void);

//-----------------------------------------------------------------------------
int kbm_display(void);
int kbm_btn0(bool_t pressed);
int kbm_btn1(bool_t pressed);

#endif // _naniBox_kuroBox_menu
