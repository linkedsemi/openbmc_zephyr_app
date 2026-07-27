/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Shell command wrapper for busctl - D-Bus introspection tool
 *
 * Runs busctl commands in a dedicated worker thread to avoid blocking the
 * shell. Connects to dbus-broker via connect_to_dbroker().
 *
 * IMPORTANT: A single persistent worker thread is created ONCE at init and
 * commands are handed off via a depth-1 message queue. This serializes
 * commands AND avoids ever calling k_thread_create() on a thread that is
 * still alive.
 *
 * Calling k_thread_create() on an already-running static k_thread
 * reinitializes its base.timeout (sys_dnode_init zeroes node.next/prev)
 * while that node is still linked in the kernel timeout_list. The next
 * sys_clock tick then faults in sys_dlist_remove() (store access fault).
 * The previous version reused a single static struct per command and hit
 * exactly that corruption when a stuck busctl thread was re-spawned.
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <string.h>
#include <stdlib.h>

/* busctl Zephyr entry point (from basu) */
extern int basu_busctl_entry(int argc, char *argv[]);

/* Worker thread stack and control */
K_THREAD_STACK_DEFINE(busctl_shell_stack, 16 * 1024);
static struct k_thread busctl_shell_thread;

/* Command handoff: one pending command at a time (depth-1 queue => serial). */
struct busctl_thread_args {
	int argc;
	char **argv;
};
K_MSGQ_DEFINE(busctl_msgq, sizeof(struct busctl_thread_args), 1, 4);

static void busctl_worker_fn(void *p1, void *p2, void *p3)
{
	struct busctl_thread_args args;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		if (k_msgq_get(&busctl_msgq, &args, K_FOREVER) != 0) {
			/* Should not happen with K_FOREVER, but stay alive. */
			continue;
		}

		basu_busctl_entry(args.argc, args.argv);

		/* Cleanup (argv was heap-allocated in cmd_busctl). */
		for (int i = 0; i < args.argc; i++) {
			free(args.argv[i]);
		}
		free(args.argv);
	}
}

static int cmd_busctl(const struct shell *sh, size_t argc, char **argv)
{
	struct busctl_thread_args args;
	char **argv_copy;

	argv_copy = malloc(sizeof(char *) * (argc + 1));
	if (!argv_copy) {
		shell_error(sh, "out of memory");
		return -ENOMEM;
	}

	for (size_t i = 0; i < argc; i++) {
		argv_copy[i] = strdup(argv[i]);
		if (!argv_copy[i]) {
			for (size_t j = 0; j < i; j++) {
				free(argv_copy[j]);
			}
			free(argv_copy);
			shell_error(sh, "out of memory");
			return -ENOMEM;
		}
	}
	argv_copy[argc] = NULL;	/* NULL-terminate like Linux argv */

	args.argc = argc;
	args.argv = argv_copy;

	/* Depth-1 queue: if the worker is still busy (e.g. a previous command
	 * is blocked on an unresponsive bus), the queue is full and we fail
	 * fast with -EBUSY instead of corrupting a live thread. */
	if (k_msgq_put(&busctl_msgq, &args, K_NO_WAIT) != 0) {
		for (size_t i = 0; i < argc; i++) {
			free(argv_copy[i]);
		}
		free(argv_copy);
		shell_error(sh, "busctl: previous command still running");
		return -EBUSY;
	}

	return 0;
}

SHELL_CMD_ARG_REGISTER(busctl, NULL,
	"busctl - D-Bus introspection tool\n"
	"Subcommands: list, status, tree, introspect, call,\n"
	"             get-property, set-property, help",
	cmd_busctl, 1, 12);

static int busctl_shell_init(void)
{
	k_thread_create(&busctl_shell_thread, busctl_shell_stack,
		K_THREAD_STACK_SIZEOF(busctl_shell_stack),
		busctl_worker_fn, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4), 0, K_NO_WAIT);
	return 0;
}
SYS_INIT(busctl_shell_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
