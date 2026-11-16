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

#ifndef SHELL_TOKENIZER_H_
#define SHELL_TOKENIZER_H_

#include <stdbool.h>
#include <stdint.h>
#include <wonderful.h>

/**
 * Character used as end-of-line by the tokenizer.
 */
#define TOKENIZER_EOL 0x7F

/**
 * Start tokenizing a shell line string. Edits the buffer in-place.
 *
 * Needs at least two free bytes after the end of the buffer.
 *
 * @param Shell line buffer.
 * @return Pointer to first token in the shell line.
 */
char *shell_token_start(char *line);
/**
 * Fetch the next token of a shell line string already processed
 * with shell_token_start - takes the return value of shell_token_start
 * or shell_token_enxt as an argument.
 *
 * @return Pointer to next token in the shell line.
 */
char *shell_token_next(char *line);
/**
 * Turn a token string to lowercase in-place.
 */
void shell_token_lower(char *token);

#endif /* SHELL_TOKENIZER_H_ */
