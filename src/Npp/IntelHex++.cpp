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

#include <vector>

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
	// If number of threads could not be determined, set it to 1
    if (max_threads == 0) {
		max_threads = 1;
	}
    HWND scintilla = help::scintilla::getCurrentInstance();
    help::scintilla::setUndoActionStart(scintilla);
	//convert all end of lines to a single matching end of line
    size_t eol_mode = ::SendMessage(scintilla, SCI_GETEOLMODE, NULL, NULL);
	::SendMessage(scintilla, SCI_CONVERTEOLS, eol_mode, NULL);
    size_t num_chars = ::SendMessage(scintilla, SCI_GETLENGTH, NULL, NULL)  + 1; // add one for \0
    size_t num_lines = ::SendMessage(scintilla, SCI_GETLINECOUNT, NULL, NULL);
    size_t cursor_position = ::SendMessage(scintilla, SCI_GETCURRENTPOS, NULL, NULL);
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
    text = new char[num_chars];
    ::SendMessage(scintilla, SCI_GETTEXT, num_chars, (LPARAM)text);
    if (text[0] == 0) {
		delete[] text;
		help::scintilla::setUndoActionEnd(scintilla);
        return;
    }
    size_t chars_per_split = size_t(double(num_chars) / max_threads);
    if (max_threads == 1 || chars_per_split < 5000) {
        //more threads than lines
        fixChecksumsDocument(text, num_chars, eol_mode);
        ::SendMessage(scintilla, SCI_SETTEXT, NULL, (LPARAM)text);
		delete[] text;
		help::scintilla::setUndoActionEnd(scintilla);
        //reset position
        ::SendMessage(scintilla, SCI_GOTOPOS, cursor_position, NULL);
        return;
	}
    else {
		std::vector<PDOCUMENT> partial_docs;
		size_t total_chars = 0, chars_this_split;
		char* start = text, * end = text;
		DWORD i;
		// Initialise threads
		for (i = 0; i < max_threads; i++) {
			PDOCUMENT doc = new DOCUMENT;
			doc->part = i;
			if (total_chars > num_chars)
				break;
			// The last thread will use all the characters that are left over
			if (i == max_threads - 1)
				chars_this_split = num_chars - total_chars;
			else
				chars_this_split = chars_per_split;
			// Except for the first thread each thread will start where the previous thread left off
			if (i > 0) {
				start = end + 1;
			}
			// Except for the last thread each thread will be about the size of chars_per_split
			if (i < max_threads - 1) {
				end = start + chars_this_split;
				// Each thread will set the end pointer to the next line
				while (*end != ':' && *end != 0) {
					end++;
				}
				// Exclude the next ':'
				if (end != NULL) {
					end--;
				}
			}
			else {
				end = &text[num_chars - 1];
			}
			chars_this_split = end - start + 1;
			char* split = new char[chars_this_split + 1];
			if (split != NULL) {
				memcpy(split, start, chars_this_split);
				split[chars_this_split] = 0;
				doc->text = split;
				doc->eol_mode = eol_mode;
				doc->characters = chars_this_split;
				HANDLE thread = CreateThread(
					NULL,                  // default security attributes
					0,                     // use default stack size  
					fixChecksumsThread,    // thread function name
					doc,                   // argument to thread function 
					0,                     // use default creation flags 
					NULL                   // returns the thread identifier
				);
				doc->thread = thread;
				if (!thread) {
					for (size_t j = 0; j < partial_docs.size(); j++) {
						WaitForSingleObject(partial_docs[j]->thread, INFINITE);
						CloseHandle(partial_docs[j]->thread);
						delete partial_docs[j]->text;
						delete partial_docs[j];
					}
					delete doc;
					delete[] split;
					fixChecksumsDocument(text, num_chars, eol_mode);
					::SendMessage(scintilla, SCI_SETTEXT, NULL, (LPARAM)text);
					delete[] text;
					help::scintilla::setUndoActionEnd(scintilla);
					::SendMessage(scintilla, SCI_GOTOPOS, cursor_position, NULL);
					return;
				}
				partial_docs.push_back(doc);
			}
			else {
				delete doc;
				continue;
			}
			// Thread was successfully created
			total_chars += chars_this_split;
		}
		delete[] text;
		total_chars = 0;
		text = (char*)calloc(num_chars + 2 * num_lines, 1);
		for (size_t j = 0; j < partial_docs.size(); j++) {
			WaitForSingleObject(partial_docs[j]->thread, INFINITE);
			CloseHandle(partial_docs[j]->thread);
			if (text != NULL) {
				memcpy(text + total_chars, partial_docs[j]->text, partial_docs[j]->characters);
				total_chars += partial_docs[j]->characters;
			}
			delete[] partial_docs[j]->text;
			delete partial_docs[j];
		}
		if (text == NULL) return;
		::SendMessage(scintilla, SCI_SETTEXT, NULL, (LPARAM)text);
		free(text);
		help::scintilla::setUndoActionEnd(scintilla);
		::SendMessage(scintilla, SCI_GOTOPOS, cursor_position, NULL); //reset position
	}
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