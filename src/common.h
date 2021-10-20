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

#ifndef HELP_H_DEFINED
#define HELP_H_DEFINED

#include <AtlBase.h>
#include <atlconv.h>

#include "Npp/PluginInterface.h"
#include "Npp/IntelHex++.h"

namespace help {

	namespace scintilla {
		HWND getCurrentInstance(void);
		void setUndoStart(HWND);
		void setUndoEnd(HWND);
		void setCursorWait(HWND);
		void setCursorNormal(HWND);
		void setUndoActionStart(HWND);
		void setUndoActionEnd(HWND);
	}

	namespace npp {
		HWND getCurrentInstance(void);
		void toggleMenuCheck(HWND, int, bool);
	}

	namespace hex {
		uint8_t hex2int(const unsigned char&);
		uint8_t int2hex_lower(const uint8_t&);
		uint8_t int2hex_upper(const uint8_t&);
		uint8_t byte2hex(const char*);
	}

	namespace intelhex {
		int16_t calculateChecksum(const char*, size_t);
		int16_t readChecksum(const char*, size_t);
		size_t calculateChecksumPosition(const char*);
		bool isValidLine(const char*, size_t);
	}
}
#endif