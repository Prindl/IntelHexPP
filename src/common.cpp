/**********************************************************
 *                                                        *
 * This file is part of the IntelHex++ plugin.            *
 * Copyright (C) 2021 Maximilian Prindl                   *
 *                                                        *
 * Last modified: 20.10.2021                              *
 *                                                        *
 **********************************************************/

//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "common.h"

extern FuncItem funcItem[nbFunc];
extern NppData nppData;
extern SETTINGS settings;


/********************************* SCINTILLA SHORTCUT FUNCTIONS *********************************/
//Gets the scintila handle
HWND help::scintilla::getCurrentInstance(void) {
	int which = -1;
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
	if (which == 0)
		return nppData._scintillaMainHandle;
	else if (which == 1)
		return nppData._scintillaSecondHandle;
	else
		return NULL;
}
//Sets the begin of the UNDO action
void help::scintilla::setUndoStart(HWND scintilla) {
	if (scintilla == NULL) {
		scintilla = help::scintilla::getCurrentInstance();
	}
	::SendMessage(scintilla, SCI_BEGINUNDOACTION, NULL, NULL);
}
//Sets the end of the UNDO action
void help::scintilla::setUndoEnd(HWND scintilla) {
	if (scintilla == NULL) {
		scintilla = help::scintilla::getCurrentInstance();
	}
	::SendMessage(scintilla, SCI_ENDUNDOACTION, NULL, NULL);
}
//Sets the cursor to wait (blue loading circle)
void help::scintilla::setCursorWait(HWND scintilla) {
	if (scintilla == NULL) {
		scintilla = help::scintilla::getCurrentInstance();
	}
	::SendMessage(scintilla, SCI_SETCURSOR, SC_CURSORWAIT, NULL);
}
//Sets the cursor to arrow (normal)
void help::scintilla::setCursorNormal(HWND scintilla) {
	if (scintilla == NULL) {
		scintilla = help::scintilla::getCurrentInstance();
	}
	::SendMessage(scintilla, SCI_SETCURSOR, SC_CURSORARROW, NULL);
}
//Combination of UNDO start and setting cursor to wait
void help::scintilla::setUndoActionStart(HWND scintilla) {
	if (scintilla == NULL) {
		scintilla = help::scintilla::getCurrentInstance();
	}
	help::scintilla::setUndoStart(scintilla);
	help::scintilla::setCursorWait(scintilla);
}
//Combination of UNDO end and setting cursor to arrow
void help::scintilla::setUndoActionEnd(HWND scintilla) {
	if (scintilla == NULL) {
		scintilla = help::scintilla::getCurrentInstance();
	}
	help::scintilla::setCursorNormal(scintilla);
	help::scintilla::setUndoEnd(scintilla);
}

/********************************* Notepad++ SHORTCUT FUNCTIONS *********************************/
//Returns the npp handle
HWND help::npp::getCurrentInstance(void) {
	return nppData._nppHandle;
}

//Gets the plugin menu item of idx and toggles the checked state
void help::npp::toggleMenuCheck(HWND npp, int idx, bool check) {
	if (npp == NULL) {
		npp = help::npp::getCurrentInstance();
	}
	::SendMessage(npp, NPPM_SETMENUITEMCHECK, funcItem[idx]._cmdID, check);
//	::CheckMenuItem(::GetMenu(npp), funcItem[idx]._cmdID, MF_BYCOMMAND | (check?MF_CHECKED:MF_UNCHECKED));
}

/********************************* CHAR/INT CONVERSION FUNCTIONS *********************************/
//Converts a hex char to a number (upper or lower case)
uint8_t help::hex::hex2int(const unsigned char& c)
{
	uint8_t value = c - 48;
	if (value >= 0 && value <= 9)
		return value;
	else if (value >= 17 && value <= 22)
		return value - 7;
	else if (value >= 49 && value <= 54)
		return value - 39;
	else return 0;
}
//Converts an integer to a hex char (lowercase)
uint8_t help::hex::int2hex_lower(const uint8_t& num)
{
	uint8_t c = 0;
	if (num < 10)
		c = num + 48;
	else
		c = num + 87;
	return c;
}
//Converts an integer to a hex char (uppercase)
uint8_t help::hex::int2hex_upper(const uint8_t& num)
{
	uint8_t c = 0;
	if (num < 10)
		c = num + 48;
	else
		c = num + 55;
	return c;
}
//Converts a byte in hex to an integer
uint8_t help::hex::byte2hex(const char* bytes)
{
	return help::hex::hex2int(bytes[0]) * 16 + help::hex::hex2int(bytes[1]);
}

/********************************* INTELHEX SHORTCUT FUNCTIONS *********************************/
//Calculates the checksum of a line in intelhex format
int16_t help::intelhex::calculateChecksum(const char* line, size_t length)
{
	int16_t checksum = 0;
	size_t num2checksum = help::intelhex::calculateChecksumPosition(line);
	if (num2checksum > length)
		return -1;
	size_t i = 0;
	if (line[0] == ':')
		i++;
	for (; i < num2checksum; i += 2)
	{
		checksum += help::hex::byte2hex(&line[i]);
	}
	return ((~checksum + 1) & 0xFF);
}
//Reads the checksum of a line in intelhex format
int16_t help::intelhex::readChecksum(const char* line, size_t length)
{
	int16_t checksum = 0;
	size_t num2checksum = help::intelhex::calculateChecksumPosition(line);
	if (num2checksum >= length)
		return -1;
	checksum = help::hex::byte2hex(&line[num2checksum]);
	return checksum & 0xFF;
}
//Calculates the position of the checksum in a line in intelhex format
size_t help::intelhex::calculateChecksumPosition(const char* line) {
	size_t len;
	if (line[0] == ':') {
		len = help::hex::byte2hex(&line[1]);
		return 2 * len + 9; //2 len, 4 addr, 2 rec, +1 for ':'
	}
	else {
		len = help::hex::byte2hex(&line[0]);
		return 2 * len + 8;
	}
}
//Checks if a line in intelhex format is valid (doesn't verify checksum)
bool help::intelhex::isValidLine(const char* line, size_t line_length)
{
	if (line == NULL)
		return false;
	//each byte uses 2 chars + the record mark : -> odd number of chars, max 523(including \r\n)
	if (line_length % 2 == 0 || line_length > 521 || line_length < 11)
		return false;
	if (line[0] != ':')
		return false;
	//validate rec type
	uint8_t rec_type = help::hex::byte2hex(&line[7]);
	if (rec_type > 5){
		return false;
	}
	//validate length to match rec type
	uint8_t len = help::hex::byte2hex(&line[1]);
	if (rec_type == 0x00){
		if (size_t(len) * 2 + 11 != line_length)
			return false;
	}
	else if (rec_type == 0x01 && (len != 0 || line_length != 11)) {
		return false;
	}
	else if ((rec_type == 2 || rec_type == 4) && (len != 2 || line_length != 15)) {
		return false; //Types 2 & 4
	}
	else if ((rec_type ==3||rec_type == 5) && (len != 4 || line_length != 19)) {
		return false; //Types 3 & 5
	}
	return true;
}
