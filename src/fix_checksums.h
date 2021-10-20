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

#ifndef CHECKSUMS_H_DEFINED
#define CHECKSUMS_H_DEFINED

#include "common.h"

void fixChecksumsDocument(char*, size_t&, size_t);
DWORD WINAPI fixChecksumsThread(void*);

#endif