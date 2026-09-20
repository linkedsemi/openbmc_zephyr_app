/* user_settings.h
 *
 * Copyright (C) 2014-2024 wolfSSL Inc.
 *
 * This file is part of wolfSSH.
 *
 * wolfSSH is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfSSH is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with wolfSSH.  If not, see <http://www.gnu.org/licenses/>.
 */

/* ---------------------------------------------------------------------------
 * Application-side wolfSSH settings.
 *
 * This is the effective settings file for wolfSSH; it mirrors what
 * CONFIG_WOLFSSL_SETTINGS_FILE="wolfssl_user_settings.h" does for wolfSSL:
 * prj.conf sets
 *
 *     CONFIG_WOLFSSH_SETTINGS_FILE="wolfssh_user_settings.h"
 *
 * so nothing under ${ZEPHYR_BASE}/subsys/shell/backends/ has to be patched any
 * more (that upstream file stays pristine and is simply unused).
 *
 * The bare file name is resolved through ${APPLICATION_CONFIG_DIR}, which the
 * wolfSSL module adds to the global include path
 * (modules/lib/wolfssl/zephyr/CMakeLists.txt) -- the same mechanism that makes
 * "wolfssl_user_settings.h" work.
 *
 * wolfSSH pulls this in from wolfssh/settings.h:
 *     #if defined(WOLFSSH_ZEPHYR) && defined(CONFIG_WOLFSSH_SETTINGS_FILE)
 *         #include CONFIG_WOLFSSH_SETTINGS_FILE
 *     #endif
 * so it is only visible in translation units that define WOLFSSH_ZEPHYR (the
 * wolfSSH module sources and the app sources; NOT the zephyr library -- see
 * subsys/shell/backends/shell_wolfssh_scp.c, which defines it itself).
 * ------------------------------------------------------------------------- */

#ifndef WOLFSSH_USER_SETTINGS_H
#define WOLFSSH_USER_SETTINGS_H


#ifdef __cplusplus
extern "C" {
#endif

#include <wolfssl/wolfcrypt/types.h>

#undef  WOLFSSH_SMALL_STACK
#define WOLFSSH_SMALL_STACK

/* --- SCP (wolfSSH only implements the server side) ----------------------
 * Turns the SSH shell endpoint into something "scp -O file <bmc>:/dir/" can
 * talk to: wolfSSH_accept() then returns WS_SCP_INIT instead of WS_SUCCESS
 * and the transfer is driven by shell_wolfssh_scp.c.
 *
 * WOLFSSH_SCP_USER_CALLBACKS is mandatory on Zephyr: the library's default
 * callbacks assume a POSIX cwd -- they WCHDIR() into the requested target and
 * afterwards open the bare file name -- while port.h maps WCHDIR to
 * z_fs_chdir(), which only *validates* that a directory exists (Zephyr has no
 * per-process cwd), and fs_open() wants an absolute path. Supplying our own
 * callbacks lets us use the Zephyr fs API directly and never touch ssh->fs.
 *
 * NO_FILESYSTEM stays defined on purpose: it keeps port.h/port.c's z_fs_* /
 * wssh_z_* glue (and the whole "default callback" code path) out of the
 * build, which is unused with WOLFSSH_SCP_USER_CALLBACKS.
 */
#undef WOLFSSH_SCP
#define WOLFSSH_SCP
#define WOLFSSH_SCP_USER_CALLBACKS

/* --- password-less userauth -------------------------------------------------
 * Without this, wolfSSH does not compile DoUserAuthRequestNone()
 * (internal.c:6143 + the dispatch at :7440 fall through to
 * SendUserAuthFailure()), so the client's initial "none" probe is answered
 * with "publickey,password" and OpenSSH/scp stops to ask for a password.
 * wsUserAuth() accepts every credential anyway (weak default in
 * shell_wolfssh.c returning WOLFSSH_USERAUTH_SUCCESS), so that prompt buys
 * nothing -- it only gets in the way: while the human is typing, the server
 * sits in NonBlockSSH_accept()'s tcp_select(LOGIN_TIMEOUT = 5 s) and then
 * drops the connection with disconnect reason 14
 * (NO_MORE_AUTH_METHODS_AVAILABLE). Accepting "none" makes ssh/scp connect
 * without any credential exchange.
 *
 * NOTE: this does not weaken anything as long as wsUserAuth() stays a
 * blanket "yes"; it becomes relevant the moment a real credential check is
 * implemented. */
#undef WOLFSSH_ALLOW_USERAUTH_NONE
#define WOLFSSH_ALLOW_USERAUTH_NONE

#undef NO_APITEST_MAIN_DRIVER
#define NO_APITEST_MAIN_DRIVER

#undef NO_TESTSUITE_MAIN_DRIVER
#define NO_TESTSUITE_MAIN_DRIVER

#undef NO_UNITTEST_MAIN_DRIVER
#define NO_UNITTEST_MAIN_DRIVER

#undef NO_MAIN_DRIVER

#undef WS_NO_SIGNAL
#define WS_NO_SIGNAL

#undef WS_USE_TEST_BUFFERS
#define WS_USE_TEST_BUFFERS

#undef NO_WOLFSSL_DIR
#define NO_WOLFSSL_DIR

#undef WOLFSSH_NO_NONBLOCKING
#define WOLFSSH_NO_NONBLOCKING

#define DEFAULT_WINDOW_SZ (128 * 128)
#define WOLFSSH_MAX_SFTP_RW 8192

/* SCP staging buffer (heap, one per SCP session). Only debug/config sized
 * files are expected here, so 8 KiB instead of the 32 KiB default is plenty
 * and saves 24 KiB of the arena for the whole transfer. */
#undef  DEFAULT_SCP_BUFFER_SZ
#define DEFAULT_SCP_BUFFER_SZ (8 * 1024)

#undef NO_FILESYSTEM
#define NO_FILESYSTEM

#ifdef __cplusplus
}
#endif

#endif
