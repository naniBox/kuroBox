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
#include "logger.h"
#include "screen.h"

//-----------------------------------------------------------------------------
/*static Thread * loggerThread;*/
uint32_t counter;

//-----------------------------------------------------------------------------
static WORKING_AREA(waLogger, 128);
static msg_t 
thLogger(void *arg) 
{
	(void)arg;
	chRegSetThreadName("Logger");
	systime_t sleep_until = chTimeNow();
	while( !chThdShouldTerminate() )
	{
		sleep_until += MS2ST(5);            // Next deadline
		// LOG IT!
		counter++;
		kbs_setCounter(counter);
		
		// chTimeNow() will roll over every ~49 days 
		// @TODO: make this code handle that
		while ( sleep_until < chTimeNow() )
			sleep_until += MS2ST(5);
		chThdSleepUntil(sleep_until);
	}
	return 0;
}
	

//-----------------------------------------------------------------------------
int kuroBoxLogger()
{
	/*loggerThread = */chThdCreateStatic(waLogger, sizeof(waLogger), NORMALPRIO, thLogger, NULL);
	
	return 0;
}
