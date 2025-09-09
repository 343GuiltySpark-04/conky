// Copyright (C) 2025 Tristan Adams
//
// This file is part of conky.
//
// conky is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// conky is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with conky.  If not, see <https://www.gnu.org/licenses/>.

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <ctime>
#include "../conky.h"
#include "../content/specials.h"
#include "../content/text_object.h"
#include "../core.h"
#include "../logging.h"

struct text_object *parse_countdown(struct text_object *obj, int arg) {
  if (!arg) return nullptr;
  obj->data.i = arg;  // store target timestamp


  obj->parse = false;

  return obj;
}

void print_countdown(struct text_object *obj, char *buf,
                     unsigned int buf_size) {
  time_t now = time(nullptr);
  time_t target = static_cast<time_t>(obj->data.i);
  long delta = target - now;

  bool counting_up = false;
  if (delta < 0) {
    delta = -delta;      // flip to positive
    counting_up = true;  // mark that we’re past the target
  }

  int days = delta / 86400;
  int hours = (delta % 86400) / 3600;
  int mins = (delta % 3600) / 60;
  int secs = delta % 60;

  if (counting_up) {
    snprintf(buf, buf_size, "+%dd %02dh %02dm %02ds", days, hours, mins, secs);
  } else {
    snprintf(buf, buf_size, "-%dd %02dh %02dm %02ds", days, hours, mins, secs);
  }
}
