/**
 * Copyright (C) 2025 Tristan Adams
 *
 * This file is part of conky.
 *
 * conky is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * conky is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with conky.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _JINJU_H
#define _JINJU_H

#pragma once
// #include "text_object.h"

// Parser: takes a text_object and a string arg (your target Unix timestamp).
struct text_object *parse_countdown(struct text_object *obj, int arg);

// Printer: writes the formatted countdown into buf.
void print_countdown(struct text_object *obj, char *buf, unsigned int buf_size);

void print_apt_installed(struct text_object *obj, char *buf,
                         unsigned int buf_size);

void print_apt_upgradable(struct text_object *obj, char *buf,
                          unsigned int buf_size);

void print_apt_local(struct text_object *obj, char *buf, unsigned int buf_size);

void print_unread_mail(struct text_object *obj, char *buf,
                       unsigned int buf_size);

#endif  // _JINJU_H
