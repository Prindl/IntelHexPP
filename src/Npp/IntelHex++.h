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

#ifndef PLUGINDEFINITION_H
#define PLUGINDEFINITION_H

#include <cstdint>
#include <fstream>
#include <sstream>
#include <cstring>
#include <thread>
// All definitions of plugin interface
#include "PluginInterface.h"

const TCHAR NPP_PLUGIN_NAME[] = TEXT("IntelHex++"); //PLUGIN NAME
const TCHAR NPP_PLUGIN_SETTINGS_FILE[] = TEXT("plugins\\IntelHex++\\IntelHex++.ini");
const TCHAR NPP_PLUGIN_SETTINGS_FILE_v7_6[] = TEXT("plugins\\IntelHex++.ini");

typedef uint8_t(*int2hex)(const uint8_t& num); //Function pointer to integer to hex conversion (upper/lower)

typedef struct {
    bool lowerCase;
    int2hex conversion_function;
} SETTINGS;

void loadSettings(void);
void saveSettings(void);

typedef struct {
    char* text;
    size_t eol_mode;
    size_t characters;
    size_t part;
    HANDLE thread;
} DOCUMENT, *PDOCUMENT;

const int nbFunc = 2; //How many plugin commands

void pluginInit(HANDLE hModule);

void pluginCleanUp(void);

void commandMenuInit(void);

void commandMenuCleanUp(void);

bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk = NULL, bool check0nInit = false);


void splitDocument(void);
void toggleLowerCase(void);
#endif //PLUGINDEFINITION_H
