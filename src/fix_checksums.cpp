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


#include "fix_checksums.h"

extern SETTINGS settings;

//Fixes the checksums of a (partial) document NOTE: Inserting is also possible but expensive (memmove)
void fixChecksumsDocument(char* text, size_t& num_chars, size_t eol_mode) {
    size_t eol_length;
    if (eol_mode == 0) // \r\n as EOL
        eol_length = 2;
    else //1 or 2 for \r or \n as EOL
        eol_length = 1;
    size_t pos = 0;
    size_t line_length = 0;
    //find the start of the file
    char* line = text, * end;
    while (pos < num_chars)
    {
        //find start of the next file
        end = strchr(line + 1, ':');
        if (end != NULL) {
            line_length = size_t(end - line) - eol_length;
            pos += line_length;
        }
        else {
            //reched EOF, end is last char -> \0
            end = &text[num_chars - 1];
            //if we have line separators, skip them
            while (*end == 0 || *end == '\n' || *end == '\r') {
                end--;
            }
            //increment end to set the pos after the last char
            line_length = ++end - line;
            pos += line_length;
        }
        if (help::intelhex::isValidLine(line, line_length)) {
            int16_t calc_checksum = help::intelhex::calculateChecksum(line, line_length);
            int16_t read_checksum = help::intelhex::readChecksum(line, line_length);
            if (calc_checksum != read_checksum && calc_checksum != -1 && read_checksum != -1) {
                //pos is right after the checksum
                text[pos - 2] = settings.conversion_function(calc_checksum / 16 & 0xFF);
                text[pos - 1] = settings.conversion_function(calc_checksum % 16);
            }
        }
        else if (line_length == help::intelhex::calculateChecksumPosition(line)) {
            int16_t calc_checksum = help::intelhex::calculateChecksum(line, line_length);
            if (calc_checksum != -1) {
                memmove(line + line_length + 2, line + line_length, num_chars - pos);
                //pos is right after the checksum
                text[pos] = settings.conversion_function(calc_checksum / 16 & 0xFF);
                text[pos + 1] = settings.conversion_function(calc_checksum % 16);
                pos += 2; num_chars += 2;
            }
        }
        //look for the beginning of the next line
        line = strchr(end, ':');
        if (line != NULL) {
            pos += eol_length;
        }
        else {
            break; //EOF REACHED
        }
    }
}

DWORD WINAPI fixChecksumsThread(void* data) {
    PDOCUMENT partial_document = (PDOCUMENT)data;
    fixChecksumsDocument(partial_document->text, partial_document->characters, partial_document->eol_mode);
    return 0;
}