/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Shell command wrappers for i2c-tools: i2cdetect / i2cget / i2cset /
 * i2cdump / i2ctransfer.
 *
 * Runs each command in a single persistent worker thread (spawned once at
 * init) and hands commands off via a depth-1 message queue. This is the
 * same safe model as busctl_shell.c: it serializes commands AND avoids ever
 * calling k_thread_create() on a thread that is still alive (which would
 * corrupt the kernel timeout_list and fault on the next clock tick).
 *
 * The i2c-tools already expose Zephyr entry points <tool>_main(argc, argv)
 * that mirror main(), so each command simply forwards the shell argv to the
 * matching entry point -- no argument reshaping needed (argv[0] is already
 * the command name, exactly what the tools expect).
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

/* Active shell for the running i2c-tools command. Consumed by the stdio
 * redirection in i2c-tools/zephyr/i2ctools_stdout.h (force-included into the
 * i2c-tools library): the tools' printf/fprintf output is routed to the
 * session that invoked the command (SSH or serial) instead of the UART
 * console. Set/cleared around each command in the worker below. */
const struct shell *g_i2ctools_shell;

/* i2c char-dev force-release (defined in zephyr-devfs i2c_fs.c). Called after
 * each i2c-tools command to clear the stuck "is_open" flag that i2cdump leaves
 * behind when it exits via exit()/longjmp. Declared here to avoid a
 * cross-module include dependency. */
extern void i2c_fs_force_release(void);

/* i2c-tools Zephyr entry points (from the i2c-tools module) */
extern int i2cdetect_main(int argc, char *argv[]);
extern int i2cget_main(int argc, char *argv[]);
extern int i2cset_main(int argc, char *argv[]);
extern int i2cdump_main(int argc, char *argv[]);
extern int i2ctransfer_main(int argc, char *argv[]);

/* setjmp target shared with the i2c-tools library (lib/smbus.c). The tool
 * entry points redirect exit() to i2c_tool_exit(), which longjmps back here
 * so a tool that calls exit() returns to this loop instead of killing the
 * worker thread. */
extern jmp_buf i2c_tool_exit_jmp;

/* Worker thread stack and control */
K_THREAD_STACK_DEFINE(i2c_tools_stack, 16 * 1024);
static struct k_thread i2c_tools_thread;

/* Command handoff: one pending command at a time (depth-1 queue => serial). */
struct i2c_tool_args {
	int (*fn)(int argc, char **argv);
	int argc;
	char **argv;
	const struct shell *sh;
};
K_MSGQ_DEFINE(i2c_tools_msgq, sizeof(struct i2c_tool_args), 1, 4);

static void i2c_tools_worker_fn(void *p1, void *p2, void *p3)
{
	struct i2c_tool_args args;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		if (k_msgq_get(&i2c_tools_msgq, &args, K_FOREVER) != 0) {
			/* Should not happen with K_FOREVER, but stay alive. */
			continue;
		}

		/* Route the tool's stdout/stderr to the invoking session.
		 * Set BEFORE setjmp so the global survives an exit() via
		 * longjmp back here. */
		g_i2ctools_shell = args.sh;

		/* Run the tool. If it calls exit(), i2c_tool_exit() longjmps
		 * back here (rc != 0) instead of terminating the thread. */
		int rc = setjmp(i2c_tool_exit_jmp);

		if (rc == 0) {
			args.fn(args.argc, args.argv);
		}

		/* Cleanup (argv was heap-allocated in the cmd handler). This
		 * runs both for a normal return and for an exit()-via-longjmp.
		 *
		 * i2c-tools may call exit() internally (e.g. i2cdump on error or
		 * early return); when caught by the setjmp/longjmp above,
		 * i2cdev_close() is never reached and the char-dev "opened" flag
		 * stays set, which would make every later open fail with
		 * -EBUSY. Force-release all i2c char-dev opens here so the next
		 * command starts clean. Safe: commands are fully serialized. */
		i2c_fs_force_release();

		for (int i = 0; i < args.argc; i++) {
			free(args.argv[i]);
		}
		free(args.argv);

		g_i2ctools_shell = NULL;
	}
}

/* Copy the shell argv and hand it to the persistent worker thread. */
static int i2c_tool_enqueue(const struct shell *sh, size_t argc, char **argv,
			    int (*fn)(int, char **))
{
	char **argv_copy;
	struct i2c_tool_args args;

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

	args.fn = fn;
	args.argc = argc;
	args.argv = argv_copy;
	args.sh = sh;

	/* Depth-1 queue: if the worker is still busy (e.g. a previous command
	 * is blocked on an unresponsive I2C device), fail fast with -EBUSY
	 * instead of corrupting a live thread. */
	if (k_msgq_put(&i2c_tools_msgq, &args, K_NO_WAIT) != 0) {
		for (size_t i = 0; i < argc; i++) {
			free(argv_copy[i]);
		}
		free(argv_copy);
		shell_error(sh, "i2c: previous command still running");
		return -EBUSY;
	}

	return 0;
}

static int cmd_i2cdetect(const struct shell *sh, size_t argc, char **argv)
{ return i2c_tool_enqueue(sh, argc, argv, i2cdetect_main); }
static int cmd_i2cget(const struct shell *sh, size_t argc, char **argv)
{ return i2c_tool_enqueue(sh, argc, argv, i2cget_main); }
static int cmd_i2cset(const struct shell *sh, size_t argc, char **argv)
{ return i2c_tool_enqueue(sh, argc, argv, i2cset_main); }
static int cmd_i2cdump(const struct shell *sh, size_t argc, char **argv)
{ return i2c_tool_enqueue(sh, argc, argv, i2cdump_main); }
static int cmd_i2ctransfer(const struct shell *sh, size_t argc, char **argv)
{ return i2c_tool_enqueue(sh, argc, argv, i2ctransfer_main); }

SHELL_CMD_ARG_REGISTER(i2cdetect, NULL,
	"i2cdetect - detect I2C chips\n"
	"Usage: i2cdetect [-y] [-a] [-q|-r] I2CBUS [FIRST LAST]\n"
	"       i2cdetect -l            (list installed I2C busses)",
	cmd_i2cdetect, 1, 40);

SHELL_CMD_ARG_REGISTER(i2cget, NULL,
	"i2cget - read from I2C chip\n"
	"Usage: i2cget [-y] I2CBUS CHIP-ADDRESS [DATA-ADDRESS [MODE]]",
	cmd_i2cget, 1, 40);

SHELL_CMD_ARG_REGISTER(i2cset, NULL,
	"i2cset - write to I2C chip\n"
	"Usage: i2cset [-y] [-m MASK] [-r] I2CBUS CHIP-ADDRESS DATA-ADDRESS VALUE[:MODE]...",
	cmd_i2cset, 1, 40);

SHELL_CMD_ARG_REGISTER(i2cdump, NULL,
	"i2cdump - examine I2C registers\n"
	"Usage: i2cdump [-y] [-r FIRST-LAST] [-s MODE] I2CBUS ADDRESS [MODE [BANK [BANKREG]]]",
	cmd_i2cdump, 1, 40);

SHELL_CMD_ARG_REGISTER(i2ctransfer, NULL,
	"i2ctransfer - send user-defined I2C messages\n"
	"Usage: i2ctransfer [-y] I2CBUS DESC [DATA] [DESC [DATA]]...",
	cmd_i2ctransfer, 1, 40);

static int i2c_tools_shell_init(void)
{
	k_thread_create(&i2c_tools_thread, i2c_tools_stack,
		K_THREAD_STACK_SIZEOF(i2c_tools_stack),
		i2c_tools_worker_fn, NULL, NULL, NULL,
		K_PRIO_PREEMPT(4), 0, K_NO_WAIT);
	return 0;
}
SYS_INIT(i2c_tools_shell_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
