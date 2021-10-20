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

#include "menuCmdID.h"
#include "Scintilla.h"
#include "../common.h"
#include "../fix_checksums.h"

// The plugin data that Notepad++ needs
FuncItem funcItem[nbFunc];

// The data of Notepad++ that you can use in your plugin commands
NppData nppData;

SETTINGS settings;

void loadSettings(void) {
	std::ifstream inputfile;
	inputfile.open(NPP_PLUGIN_SETTINGS_FILE);
	if (!inputfile.is_open()) {
		inputfile.open(NPP_PLUGIN_SETTINGS_FILE_v7_6);
	}
	if (inputfile.is_open()) {
		int tmp;
		inputfile >> tmp;
		inputfile.close();
		settings.lowerCase = bool(tmp);
	}
	else {
		settings.lowerCase = true;
		saveSettings(); //Create new settings file if it couldn't be opened
	}
	if (settings.lowerCase) {
		settings.conversion_function = help::hex::int2hex_lower;
	}
	else {
		settings.conversion_function = help::hex::int2hex_upper;
	}
}

void saveSettings(void) {
	std::ofstream outputfile;
	outputfile.open(NPP_PLUGIN_SETTINGS_FILE);
	if (!outputfile.is_open()) {
		outputfile.open(NPP_PLUGIN_SETTINGS_FILE_v7_6);
	}
	if (outputfile.is_open()) {
		outputfile << int(settings.lowerCase) << std::endl;
		outputfile.close();
	}
}

// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE /*hModule*/)
{
	loadSettings();
}

// Here you can do the clean up, save the parameters (if any) for the next session
void pluginCleanUp()
{
	saveSettings();
}

void commandMenuInit() {
    ShortcutKey *shKey = new ShortcutKey;
    shKey->_isAlt = true;
    shKey->_isCtrl = false;
    shKey->_isShift = false;
    shKey->_key = 0x46; //VK_F
	setCommand(0, TEXT("Lower Case"), toggleLowerCase, NULL, settings.lowerCase);
	setCommand(1, TEXT("Fix Checksums"), splitDocument, shKey, false);
}

void commandMenuCleanUp() {
    delete funcItem[1]._pShKey;
}

bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) {
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
	funcItem[index]._init2Check = check0nInit;
	funcItem[index]._pShKey = sk;

	return true;
}

void splitDocument(void) {
	size_t max_threads = std::thread::hardware_concurrency();
	HWND scintilla = help::scintilla::getCurrentInstance();
	size_t num_chars = ::SendMessage(scintilla, SCI_GETLENGTH, NULL, NULL)+1; // add one for \0
	size_t num_lines = ::SendMessage(scintilla, SCI_GETLINECOUNT, NULL, NULL);
	size_t cursor_position = ::SendMessage(scintilla, SCI_GETCURRENTPOS, NULL, NULL);
	size_t eol_mode = ::SendMessage(scintilla, SCI_GETEOLMODE, NULL, NULL);
	size_t error_code = ::SendMessage(scintilla, SCI_GETSTATUS, NULL, NULL);
    if (error_code != 0) {
        if (error_code < 1000) {
            if (error_code == 1)
                ::MessageBox(scintilla, TEXT("Something went wrong."), TEXT("Generic Failure"), MB_OK);
            else if (error_code == 2)
                ::MessageBox(scintilla, TEXT("There was an error allocating memory, your memory might be exhausted."), TEXT("Memory Error"), MB_OK);
            return;
        }// else warning
    }
	char* text = NULL;
	if (max_threads == 0) {
		text = new char[num_chars + 2*num_lines];
	}
	else {
		text = new char[num_chars];
	}
	help::scintilla::setUndoActionStart(scintilla);
    //convert all end of lines to a single matching end of line
    ::SendMessage(scintilla, SCI_CONVERTEOLS, eol_mode, NULL);
    ::SendMessage(scintilla, SCI_GETTEXT, num_chars, (LPARAM)text);
    if (text[0] == 0) {
		delete[] text;
		help::scintilla::setUndoActionEnd(scintilla);
        return;
    }
	size_t chars_per_split = size_t(double(num_chars) / max_threads);
	if (max_threads == 0 || chars_per_split < 5000) {
        //more threads than lines
        fixChecksumsDocument(text, num_chars, eol_mode);
        ::SendMessage(scintilla, SCI_SETTEXT, NULL, (LPARAM)text);
		delete[] text;
		help::scintilla::setUndoActionEnd(scintilla);
        //reset position
        ::SendMessage(scintilla, SCI_GOTOPOS, cursor_position, NULL);
        return;
    }
    HANDLE *threads = new HANDLE[max_threads];
    PDOCUMENT *partial_doc = new PDOCUMENT[max_threads];
	char** splits = new char*[max_threads];
    DWORD *threadIds = new DWORD[max_threads];
	size_t total_chars = 0, chars_this_split;
    char* start = text, *end = text;
	DWORD i = 0;
    for (; i < max_threads; i++) {
		partial_doc[i] = new DOCUMENT;
		partial_doc[i]->part = i;
        if (total_chars > num_chars)
            break;
        if (i == max_threads - 1)
            chars_this_split = num_chars - total_chars;
        else
            chars_this_split = chars_per_split;
        if (i > 0) start = end + 1;
        if (i < max_threads - 1)
            end = start + chars_this_split;
        else
            end = &text[num_chars - 1];
		//Jump to next line
		while (*end != ':' && *end != 0) {
			end++; chars_this_split++;
		}
        if (end != NULL) end--;
		splits[i] = new char[chars_this_split + 1 + num_lines*3/max_threads];
		if (splits[i] != NULL) {
			memcpy(splits[i], start, chars_this_split);
			end = start + chars_this_split;
			end[0] = 0;
			partial_doc[i]->text = splits[i];
			partial_doc[i]->eol_mode = eol_mode;
			partial_doc[i]->characters = chars_this_split;
			total_chars += chars_this_split;
			threads[i] = CreateThread(
				NULL,                   // default security attributes
				0,                      // use default stack size  
				fixChecksumsThread,    // thread function name
				partial_doc[i],         // argument to thread function 
				0,                      // use default creation flags 
				&threadIds[i]			// returns the thread identifier
			);
		}
		else {
			threads[i] = NULL;
		}
        if (threads[i] == NULL) {
			WaitForMultipleObjects(i, threads, TRUE, INFINITE);
			for (DWORD j = 0; j < i; j++) {
				CloseHandle(threads[j]);
				delete[] partial_doc[j]->text;
				delete partial_doc[j];
			}
			delete[] partial_doc; delete[] splits; delete[] threadIds;
			fixChecksumsDocument(text, num_chars, eol_mode);
			::SendMessage(scintilla, SCI_SETTEXT, NULL, (LPARAM)text);
			delete[] text;
			help::scintilla::setUndoActionEnd(scintilla);
			::SendMessage(scintilla, SCI_GOTOPOS, cursor_position, NULL);
			ExitProcess(3);
        }
    }
	delete[] text;
    WaitForMultipleObjects(i, threads, TRUE, INFINITE);
	total_chars = 0;
	text = (char *) calloc(num_chars + 2 * num_lines, 1);
    for (DWORD j = 0; j < i; j++) {
        CloseHandle(threads[j]);
		if (text != NULL) {
			memcpy(text + total_chars, partial_doc[j]->text, partial_doc[j]->characters);
			total_chars += partial_doc[j]->characters;
		}
		delete[] partial_doc[j]->text;
		delete partial_doc[j];
    }
    delete[] partial_doc; delete[] splits; delete[] threadIds;
	if(text == NULL) ExitProcess(3);
    ::SendMessage(scintilla, SCI_SETTEXT, NULL, (LPARAM)text);
	free(text);
	help::scintilla::setUndoActionEnd(scintilla);
    ::SendMessage(scintilla, SCI_GOTOPOS, cursor_position, NULL); //reset position
}

void toggleLowerCase(void) {
	settings.lowerCase = !settings.lowerCase;
	if (settings.lowerCase) {
		settings.conversion_function = help::hex::int2hex_lower;
	}
	else {
		settings.conversion_function = help::hex::int2hex_upper;
	}
	help::npp::toggleMenuCheck(NULL, 0, settings.lowerCase);
}