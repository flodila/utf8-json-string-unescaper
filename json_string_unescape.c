/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*  unescape_json_string
 *  ====================
 *  Copyright (C) 2018 Flodila
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

typedef enum {
    txt,       // normal text (initial state)
    escaped,   // just after a reverse solidus
    unicode,   // for four bytes after \u (reverse solidus + u)
    utf16_i1,  // 1st char after UTF-16 High-Surrogate (reverse solidus)
    utf16_i2,  // 2nd char after UTF-16 High-Surrogate (u)
    utf16_ls   // UTF-16 Low-Surrogate for for four bytes after utf16_ls
} unescape_json_string_state;

static int u8_escape_codepoint(long codepoint, char **target)
{
    int len = 0;
    char *p = *target;

    if (codepoint <= 0) {
        // Not writing a null byte.
        return 0;
    }
    else if (codepoint < 0x80L) {
        p[len++] = (char) codepoint;
    }
    else if (codepoint < 0x800l) {
        p[len++] = (char) (0xC0l | codepoint >> 6 & 0x1Fl);
        p[len++] = (char) (0x80l | codepoint & 0x3Fl);
    }
    else if (codepoint < 0x10000l) {
        p[len++] = (char) (0xE0l | codepoint >> 12 & 0xFl);
        p[len++] = (char) (0x80l | codepoint >> 6 & 0x3Fl);
        p[len++] = (char) (0x80l | codepoint & 0x3Fl);
    }
    else if (codepoint < 0x110000l) {
        p[len++] = (char) (0xF0l | codepoint >> 18 & 0x7l);
        p[len++] = (char) (0x80l | codepoint >> 12 & 0x3Fl);
        p[len++] = (char) (0x80l | codepoint >> 6 & 0x3Fl);
        p[len++] = (char) (0x80l | codepoint & 0x3Fl);
    }
    else {
        // That 's not unicode.
        return 0;
    }

    return len;
}

/*
 * Unescapes a UTF-8 encoded JSON string value.
 * See http://www.ietf.org/rfc/rfc4627.txt
 * The input string (buf) is expected to be UTF-8 encoded.
 * So will be the output (unesc_buf).
 * The number of chars written to the unesc_buf is at most buf_len and will be
 * returned. So, you'll be fine to allocate buf_len + 1 (trailing zero) chars
 * for unesc_buf.
 */
static int u8_unescape_json_string(char *buf, int buf_len, char **unesc_buf)
{
    int unesc_len = 0;
    int i;
    char ch;
    char *p = *unesc_buf;
    char *pt;

    char uxxxx[] = "xxxx"; // 4HEXDIG buffer for unicode sequences
    int uxxxx_offset;      // current offset in uxxxx
    long u1;               // UTF-16 Low-Surrogate
    long u2;               // UTF-16 High-Surrogate
    long codepoint;        // current Unicode code point

    unescape_json_string_state state = txt;

    for (i = 0; i < buf_len; i++) {
        ch = buf[i];
        if (state == txt) {
            if (ch == '\\') {
                state = escaped;
            }
            else {
                p[unesc_len++] = ch;
            }
        }
        else if (state == escaped) {
            if (ch == 'u') {
                uxxxx_offset = 0;
                state = unicode;
            }
            else {
                switch(ch) {
                    case 'n': p[unesc_len++] = '\n'; break; // line feed
                    case 't': p[unesc_len++] = '\t'; break; // tab
                    case 'r': p[unesc_len++] = '\r'; break; // carriage return
                    case 'b': p[unesc_len++] = '\b'; break; // backspace
                    case 'f': p[unesc_len++] = '\f'; break; // form feed
                    default: p[unesc_len++] = ch; break; // anything else
                }
                state = txt;
            }
        }
        else if (state == unicode) {
            uxxxx[uxxxx_offset++] = ch;
            if (uxxxx_offset == 4) {
                codepoint = strtol(uxxxx, NULL, 16);
                // U' = yyyyyyyyyyxxxxxxxxxx
                // W1 = 110110yyyyyyyyyy
                if ((codepoint & 0xFC00l) == 0xD800l) {
                    // it is a UTF-16 Low-Surrogate
                    u1 = codepoint;
                    state = utf16_i1;
                }
                else {
                    // Basic Multilingual Plane
                    pt = &p[unesc_len];
                    unesc_len += u8_escape_codepoint(codepoint, &pt);
                    state = txt;
                }
            }
        }
        else if (state == utf16_i1) {
            if (ch == '\\') {
                state = utf16_i2;
            }
            else {
                p[unesc_len++] = ch;
                state = txt;
            }
        }
        else if (state == utf16_i2) {
            if (ch == 'u') {
                uxxxx_offset = 0;
                state = utf16_ls;
            }
            else {
                switch(ch) {
                    case 'n': p[unesc_len++] = '\n'; break; // line feed
                    case 't': p[unesc_len++] = '\t'; break; // tab
                    case 'r': p[unesc_len++] = '\r'; break; // carriage return
                    case 'b': p[unesc_len++] = '\b'; break; // backspace
                    case 'f': p[unesc_len++] = '\f'; break; // form feed
                    default: p[unesc_len++] = ch; break; // anything else
                }
                state = txt;
            }
        }
        else if (state == utf16_ls) {
            uxxxx[uxxxx_offset++] = ch;
            if (uxxxx_offset == 4) {
                codepoint = strtol(uxxxx, NULL, 16);
                // U' = yyyyyyyyyyxxxxxxxxxx
                // W2 = 110111xxxxxxxxxx
                if ((codepoint & 0xFC00l) == 0xDC00l) {
                    // it is a UTF-16 High-Surrogate
                    u2 = codepoint;
                    codepoint = ((u1 & 0x3FFl) << 10 | u2 & 0x3FFl) + 0x10000l;
                    printf("codepoint: %lx\n", codepoint);
                }
                pt = &p[unesc_len];
                unesc_len += u8_escape_codepoint(codepoint, &pt);
                state = txt;
            }
        }
        else {
            return -1; // panic: unknown state
        }
    }
    p[unesc_len] = '\0';
    return unesc_len;
}

int main(int argc, char *argv[])
{
    char *input;
    char *output;
    int input_len;
    int output_len;

    if (argc < 2) {
        printf("Argument expected.\n");
        return 1;
    }
    input = argv[1];

    printf("Input:\n------\n%s\n\n", input);

    input_len = strlen(input);

    // allocate memory for output
    output = (char *) calloc((input_len + 1), sizeof(char));

    // call unescape function
    output_len = u8_unescape_json_string(input, input_len, &output);

    printf("Output:\n-------\n%s\n\n", output);

    return 0;
}
