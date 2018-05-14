/*
 * shadowsocks.h - Header files of library interfaces
 *
 * Copyright (C) 2013 - 2016, Max Lv <max.c.lv@gmail.com>
 *
 * This file is part of the shadowsocks-libev.
 * shadowsocks-libev is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * shadowsocks-libev is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shadowsocks-libev; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _SHADOWSOCKS_H
#define _SHADOWSOCKS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*shadowsocks_cb) (int fd, void*);

/*
 * Create and start a shadowsocks local server.
 *
 * Calling this function will block the current thread forever if the server
 * starts successfully.
 *
 * Make sure start the server in a separate process to avoid any potential
 * memory and socket leak.
 *
 * If failed, -1 is returned. Errors will output to the log file.
 */
int start_ss_local_server(int argc, char **argv, shadowsocks_cb cb, void *data);

#ifdef __cplusplus
}
#endif

// To stop the service on posix system, just kill the daemon process
// kill(pid, SIGKILL);
// Otherwise, If you start the service in a thread, you may need to send a signal SIGUSER1 to the thread.
// pthread_kill(pthread_t, SIGUSR1);

#endif // _SHADOWSOCKS_H
