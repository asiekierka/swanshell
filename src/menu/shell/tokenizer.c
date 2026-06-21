/**
 * Copyright (c) 2026 Adrian Siekierka
 *
 * swanshell is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * swanshell is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with swanshell. If not, see <https://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include "tokenizer.h"

char *shell_token_start(char *line) {
    char *pos = line;
    char quote = '\0';
    uint16_t offset = 0;

    char c;
    while ((c = *pos)) {
        bool skip = false;

        if (c == '\'' || c == '"') {
            if (quote == '\0') {
                // begin quote
                quote = c;
                skip = true;
            } else if (quote == c) {
                // end quote
                quote = 0;
                skip = true;
            } else {
                // mismatched quotes are not supported
                return NULL;
            }
        } else if (c == '\\') {
            // escaped character
            offset++; pos++; c = *pos;
        } else if (!quote && (c == ' ' || c == '\t')) {
            // next argument
            c = 0;
        }

        if (skip) {
            offset++;
        } else {
            if (c == TOKENIZER_EOL) {
                // invalid character - cannot copy
                return NULL;
            }
            *(pos - offset) = c;
        }
        pos++;
    }

    *(pos - offset) = 0;
    pos++;
    *(pos - offset) = TOKENIZER_EOL;
    return line;
}

char *shell_token_next(char *line) {
    while (*line != 0 && *line != TOKENIZER_EOL) {
        line++;
    }
    while (*line == 0) {
        line++;
    }
    if (*line == TOKENIZER_EOL) {
        return NULL;
    } else {
        return line;
    }
}

void shell_token_lower(char *token) {
    while (*token) {
        *token = tolower(*token);
        token++;
    }
}
