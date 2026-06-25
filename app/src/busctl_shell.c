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
 * threads would race on the broker's peer tree and dispatch context.
 *
 * IMPORTANT: Use an atomic flag instead of a semaphore to avoid K_SEM_DEFINE
 * initialization ordering issues. The flag is checked-and-set atomically in
 * the shell handler and cleared in the background thread. A sentinel flag
 * prevents permanent lock if the thread crashes without releasing. */
static atomic_t busctl_running = ATOMIC_INIT(0);
static struct k_timer busctl_abort_timer;

/* Forward declaration */
static void busctl_abort_handler(struct k_timer *timer);

/* Thread arguments: copy of argc/argv */
struct busctl_thread_args {
	int argc;
	char **argv;
};

/* Watchdog: if busctl thread crashes before releasing the lock,
 * auto-release after 30s so the shell remains usable. The watchdog
 * runs for the entire thread lifetime — it is NOT stopped at thread
 * start, only cancelled AFTER atomic_clear. This ensures that ANY
 * crash (in basu_busctl_entry, cleanup, free, etc.) triggers recovery. */
static void busctl_abort_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	printk("busctl: watchdog timeout - releasing stuck lock\n");
	atomic_clear(&busctl_running);
}

static void busctl_thread_fn(void *p1, void *p2, void *p3)
{
	struct busctl_thread_args *args = (struct busctl_thread_args *)p1;

	/* Watchdog is already running (started in cmd_busctl). Keep it
	 * running as a crash safety net — it fires only if this thread
	 * crashes or hangs, auto-releasing the lock. */

	basu_busctl_entry(args->argc, args->argv);

	/* Cleanup */
	for (int i = 0; i < args->argc; i++) {
		free(args->argv[i]);
	}
	free(args->argv);
	free(args);

	/* Release lock FIRST, then stop watchdog */
	atomic_clear(&busctl_running);
	k_timer_stop(&busctl_abort_timer);
}

static int cmd_busctl(const struct shell *sh, size_t argc, char **argv)
{
	struct busctl_thread_args *args;
	char **argv_copy;

	/* Serialize busctl commands using atomic flag. This is more reliable
	 * than a semaphore because it avoids K_SEM_DEFINE initialization
	 * ordering dependencies. The watchdog timer prevents permanent lock
	 * if the background thread crashes. */
	if (atomic_cas(&busctl_running, 0, 1) == 0) {
		shell_error(sh, "busctl: previous command still running");
		return -EBUSY;
	}

	/* Start watchdog: auto-release lock if thread crashes or hangs.
	 * The watchdog runs during the entire thread lifetime (not stopped
	 * until after atomic_clear), so even crashes inside basu_busctl_entry,
	 * cleanup, or free() are caught. 120s accounts for slow services
	 * like EntityManager (large introspect XML) plus multiple unresponsive
	 * services that each need ~10s before fast-fail kicks in. */
	k_timer_start(&busctl_abort_timer, K_SECONDS(120), K_NO_WAIT);

	/* Allocate and copy arguments for the background thread */
	args = malloc(sizeof(*args));
	if (!args) {
		atomic_clear(&busctl_running);
		k_timer_stop(&busctl_abort_timer);
		shell_error(sh, "out of memory");
		return -ENOMEM;
	}

	/* Allocate one extra slot for NULL terminator (like Linux argv) */
	argv_copy = malloc(sizeof(char *) * (argc + 1));
	if (!argv_copy) {
		atomic_clear(&busctl_running);
		k_timer_stop(&busctl_abort_timer);
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
			atomic_clear(&busctl_running);
			k_timer_stop(&busctl_abort_timer);
			free(argv_copy);
			free(args);
			shell_error(sh, "out of memory");
			return -ENOMEM;
		}
	}
	argv_copy[argc] = NULL;  /* NULL-terminate like Linux argv */

	args->argc = argc;
	args->argv = argv_copy;

	/* Spawn a new thread to run busctl
	 * Note: if k_thread_create() fails (silently on some Zephyr
	 * versions), the 10-second watchdog timer will release the lock. */
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

static int busctl_shell_init(void)
{
	k_timer_init(&busctl_abort_timer, busctl_abort_handler, NULL);
	return 0;
}
SYS_INIT(busctl_shell_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
