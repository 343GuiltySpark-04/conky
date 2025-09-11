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
#include <array>
#include <cstdio>
#include <ctime>
#include <iostream>
#include "../conky.h"
#include "../content/specials.h"
#include "../content/text_object.h"
#include "../core.h"
#include "../logging.h"
#undef Status
#include <vmime/vmime.hpp>
#include "../lua/setting.hh"

static int run_installed = 1800;
static int run_upgradable = 1810;
static int run_local = 1820;
static int run_unread_mail = 60;
int apt_installed_loop_counter = 0;
int apt_upgradable_loop_counter = 0;
int apt_local_loop_counter = 0;
int unread_mail_loop_counter = 0;
bool apt_installed_bootstrap = true;
bool apt_upgradable_bootstrap = true;
bool apt_local_bootstrap = true;
bool imap_unread_bootstrap = true;

std::string username;
std::string password;

std::array<int, 3> apt_status = {0, 0, 0};

std::array<int, 3> mail_count = {0, 0, 0};

struct text_object *parse_countdown(struct text_object *obj, int arg) {
  if (!arg) return nullptr;
  obj->data.i = arg;  // store target timestamp

  obj->parse = false;

  return obj;
}

// No-argument parse function
struct text_object *parse_packages(struct text_object *obj, int arg) {
  (void)arg;
  return obj;
}

struct text_object *parse_mail(struct text_object *obj, int arg) {
  (void)arg;
  return obj;
}

void read_email_cred() {
  const char *addr_env = std::getenv("PROTON_BRIDGE_ADDR");
  const char *pass_env = std::getenv("PROTON_BRIDGE_PWD");
  username = addr_env ? addr_env : "";
  password = pass_env ? pass_env : "";
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

// Helper to run a shell command and return numeric output
static int run_command_count(const char *cmd) {
  std::array<char, 128> buffer;
  std::string result;

  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) return 0;

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }

  while (!result.empty() && isspace(result.back())) result.pop_back();
  return result.empty() ? 0 : std::stoi(result);
}

// --- Individual text object callbacks ---

void print_apt_installed(struct text_object *obj, char *buf,
                         unsigned int buf_size) {
  (void)obj;

  if (apt_installed_bootstrap == true) {
    apt_status.at(0) = run_command_count("dpkg -l | grep '^ii' | wc -l");

    apt_installed_bootstrap = false;
  }

  if (apt_installed_loop_counter == run_installed) {
    apt_status.at(0) = run_command_count("dpkg -l | grep '^ii' | wc -l");

    apt_installed_loop_counter = 0;
  } else {
    apt_installed_loop_counter++;
  }

  snprintf(buf, buf_size, "%d", apt_status.at(0));
}

void print_apt_upgradable(struct text_object *obj, char *buf,
                          unsigned int buf_size) {
  (void)obj;

  if (apt_upgradable_bootstrap == true) {
    apt_status.at(1) = run_command_count(
        "apt list --upgradable 2>/dev/null | grep -v Listing | wc -l");

    apt_upgradable_bootstrap == false;
  }

  if (apt_upgradable_loop_counter == run_upgradable) {
    apt_status.at(1) = run_command_count(
        "apt list --upgradable 2>/dev/null | grep -v Listing | wc -l");

    apt_upgradable_loop_counter = 0;
  } else {
    apt_upgradable_loop_counter++;
  }

  snprintf(buf, buf_size, "%d", apt_status.at(1));
}

void print_apt_local(struct text_object *obj, char *buf,
                     unsigned int buf_size) {
  (void)obj;

  if (apt_local_bootstrap == true) {
    apt_status.at(2) = run_command_count(
        "apt list --installed 2>/dev/null | grep '\\[local\\]' | wc -l");

    apt_local_bootstrap = false;
  }

  if (apt_local_loop_counter == run_local) {
    apt_status.at(2) = run_command_count(
        "apt list --installed 2>/dev/null | grep '\\[local\\]' | wc -l");

    apt_local_loop_counter = 0;

  } else {
    apt_local_loop_counter++;
  }

  snprintf(buf, buf_size, "%d", apt_status.at(2));
}

int get_unread_count() {
  if (imap_unread_bootstrap == true) {
    read_email_cred();

    imap_unread_bootstrap = false;
  }

  try {
    std::string imap_url =
        "imap://" + username + ":" + password + "@127.0.0.1:1143";

    auto sess = vmime::net::session::create();
    auto store = sess->getStore(vmime::utility::url(imap_url));
    store->connect();

    vmime::utility::path inboxPath("INBOX");
    auto inbox = store->getFolder(inboxPath);
    inbox->open(vmime::net::folder::MODE_READ_ONLY);

    // VMime v9 synchronous status call

    auto status = inbox->getStatus();
    vmime::size_t total = status->getMessageCount();
    vmime::size_t unseen = status->getUnseenCount();

    inbox->close(true);
    store->disconnect();

    return static_cast<int>(unseen);

  } catch (const std::exception &e) {
    std::cerr << "Error fetching unread count: " << e.what() << std::endl;
    return 0;
  }
}

void print_unread_mail(struct text_object *obj, char *buf,
                       unsigned int buf_size) {
  (void)obj;

  if (imap_unread_bootstrap == true) {
    mail_count.at(0) = get_unread_count();
    snprintf(buf, buf_size, "%d", mail_count.at(0));
  }

  if (unread_mail_loop_counter == run_unread_mail) {
    mail_count.at(0) = get_unread_count();
    snprintf(buf, buf_size, "%d", mail_count.at(0));

  } else {
    unread_mail_loop_counter++;
  }
  snprintf(buf, buf_size, "%d", mail_count.at(0));
}
