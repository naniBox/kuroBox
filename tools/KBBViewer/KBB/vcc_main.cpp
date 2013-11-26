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

#include <iostream>
#include "KBB.h"

//-----------------------------------------------------------------------------
void printPacket(const KBB_Packet * packet)
{
	ASSERT(packet);

	const kbb_header_t & h = packet->header();
	switch( h.msg_class )
	{
	case KBB_CLASS_HEADER: 
		printf("Header packet class: 0x%02x, subclass 0x%02x\n", h.msg_class, h.msg_subclass);
		break;
	case KBB_CLASS_DATA: 
		{
			if ( h.msg_subclass == KBB_SUBCLASS_DATA_01 )
			{
				const KBB_02_01 * data = dynamic_cast<const KBB_02_01 *>(packet);
				if ( data )
				{
					const kbb_02_01_t & _02_01 = data->packet();
					printf("Data packet class: 0x%02x, subclass 0x%02x, count: %d\n", h.msg_class, h.msg_subclass, _02_01.msg_num);
				}
			}
		}
		break;
	}
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	if ( argc != 2 )
	{
		std::cout << "Usage: kbb <kbb_file>" << std::endl << std::endl;
		return -1;
	}
	KBB kbb;
	if ( kbb.open( argv[1] ) )
	{
		std::cout << "Opened [" << argv[1] << "]" << std::endl;
	}
	else
	{
		std::cout << "Error opening [" << argv[1] << "]" << std::endl;
		return -1;
	}

	/*
	while ( true )
	{
		const KBB_Packet * packet = kbb.next();

		if ( !packet )
		{
			printf("Error reading packet\n");
			break;
		}

		// stuff here
		printPacket(packet);
	}
	*/

	for( int i = 0 ; i < 10 ; i++ )
	{
		kbb.seek(i * 100);
		const KBB_Packet * packet = kbb.next();

		if ( !packet )
		{
			printf("Error reading packet\n");
			break;
		}

		printPacket(packet);
	}


	return 0;
}
