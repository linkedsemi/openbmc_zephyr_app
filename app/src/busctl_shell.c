/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Shell command wrapper for busctl - D-Bus introspection tool
 *
 * Runs busctl commands in a separate thread to avoid blocking the shell.
 * Connects to dbus-broker via connect_to_dbroker().
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <string.h>
#include <stdlib.h>

/* busctl Zephyr entry point (from basu) */
extern int basu_busctl_entry(int argc, char *argv[]);

/* Thread stack and control */
K_THREAD_STACK_DEFINE(busctl_shell_stack, 16 * 1024);
static struct k_thread busctl_shell_thread;

/* Serialization semaphore: each busctl command runs in its own thread
 * (k_thread_create with K_NO_WAIT). Without serialization, concurrent
 * threads calling socketpool_add_peer_to_broker / disconnect_from_dbroker
 * race on the broker's peer tree and dispatch context, causing crashes. */
static K_SEM_DEFINE(busctl_serial_sem, 1, 1);

/* Thread arguments: copy of argc/argv */
struct busctl_thread_args {
	int argc;
	char **argv;
};

static void busctl_thread_fn(void *p1, void *p2, void *p3)
{
	struct busctl_thread_args *args = (struct busctl_thread_args *)p1;

	/* busctl will use printk for output (macro-defined in busctl.c) */
	basu_busctl_entry(args->argc, args->argv);

	/* Cleanup */
	for (int i = 0; i < args->argc; i++) {
		free(args->argv[i]);
	}
	free(args->argv);
	free(args);

	/* Release serialization semaphore so next busctl command can run */
	k_sem_give(&busctl_serial_sem);
}

static int cmd_busctl(const struct shell *sh, size_t argc, char **argv)
{
	struct busctl_thread_args *args;
	char **argv_copy;

	/* Serialize busctl commands: reject if a previous command is still
	 * running in its background thread. This prevents concurrent access
	 * to broker peer tree / dispatch context from multiple threads. */
	if (k_sem_take(&busctl_serial_sem, K_NO_WAIT) != 0) {
		shell_error(sh, "busctl: previous command still running");
		return -EBUSY;
	}

	/* Allocate and copy arguments for the background thread */
	args = malloc(sizeof(*args));
	if (!args) {
		k_sem_give(&busctl_serial_sem);
		shell_error(sh, "out of memory");
		return -ENOMEM;
	}

	/* Allocate one extra slot for NULL terminator (like Linux argv) */
	argv_copy = malloc(sizeof(char *) * (argc + 1));
	if (!argv_copy) {
		k_sem_give(&busctl_serial_sem);
		free(args);
		shell_error(sh, "out of memory");
		return -ENOMEM;
	}

	for (size_t i = 0; i < argc; i++) {
		argv_copy[i] = strdup(argv[i]);
		if (!argv_copy[i]) {
			for (size_t j = 0; j < i; j++) {
				free(argv_copy[j]);
			}
			k_sem_give(&busctl_serial_sem);
			free(argv_copy);
			free(args);
			shell_error(sh, "out of memory");
			return -ENOMEM;
		}
	}
	argv_copy[argc] = NULL;  /* NULL-terminate like Linux argv */

	args->argc = argc;
	args->argv = argv_copy;

	/* Spawn a new thread to run busctl */
	k_thread_create(&busctl_shell_thread, busctl_shell_stack,
		K_THREAD_STACK_SIZEOF(busctl_shell_stack),
		busctl_thread_fn, args, NULL, NULL,
		K_PRIO_PREEMPT(4), 0, K_NO_WAIT);

	return 0;
}

SHELL_CMD_ARG_REGISTER(busctl, NULL,
	"busctl - D-Bus introspection tool\n"
	"Subcommands: list, status, tree, introspect, call,\n"
	"             get-property, set-property, help",
	cmd_busctl, 1, 12);
