/*
License:
	IOTA_Client is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	IOTA_Client is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with IOTA_Client.  If not, see <http://www.gnu.org/licenses/>.

Description:
	This software is used to create serial protocol between a STM32F103
	processor and an ESP8266-12 wifi device, interface with an OLED and
	Rotary encoder for HMI.

Authors: 
	Mustafa Ozcelikors 
		thewebblog.net
		github.com/mozcelikors
		<mozcelikors@gmail.com>

*/
#include "MO_Commands.h"
#include "string_islemleri.h"

Receive_Command_Type ParseReceivedCommand(uint8_t * command)
{
		if (command[0] == 'W' && command[1] == 'W')
		{
				return _WW_;
		}
		else if (command[0] == 'W' && command[1] == 'S')
		{
				return _WS_;
		}
		else if (command[0] == 'W' && command[1] == 'F')
		{
				return _WF_;
		}
		else if (command[0] == 'W' && command[1] == 'I')
		{
				return _WI_;
		}
		else if (command[0] == 'I' && command[1] == 'D')
		{
				return _ID_;
		}
		else if (command[0] == 'H' && command[1] == 'I')
		{
				return _HI_;
		}
		else if (command[0] == 'H' && command[1] == 'S')
		{
				return _HS_;
		}
		else if (command[0] == 'H' && command[1] == 'W')
		{
				return _HW_;
		}
		else if (command[0] == 'H' && command[1] == 'F')
		{
				return _HF_;
		}
		else if (command[0] == 'C' && command[1] == 'S')
		{
				return _CS_;
		}
		else if (command[0] == 'R' && command[1] == 'S')
		{
				return _RS_;
		}
		else if (command[0] == 'R' && command[1] == 'W')
		{
				return _RW_;
		}
		else if (command[0] == 'H' && command[1] == 'H')
		{
				return _HH_;
		}
		else if (command[0] == 'S' && command[1] == 'S')
		{
				return _SS_;
		}
		else
		{
				//Dont do nothin' if the command is not valid.
				return _ID_;
		}
		//Clear received command buffer.. after processing the info
}

