#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>

#include "utils.h"
#include "options.h"
#include "ipsets.h"
#include "ipset_entries.h"


static struct state* run_state = NULL;
static struct state* cfg_state = NULL;

static bool
build_state(bool runtime)
{
	struct state *state = NULL;
	struct uci_package *p = NULL;
	FILE *sf;

	state = calloc(1, sizeof(*state));
	if (!state)
		error("Out of memory");

	state->uci = uci_alloc_context();

	if (!state->uci)
		error("Out of memory");

	/* Set /var/state overlay */
	uci_set_savedir(state->uci, "/var/state");

	if (runtime)
	{
		sf = fopen(STATEFILE, "r");

		if (sf)
		{
			uci_import(state->uci, sf, "state", &p, true);
			fclose(sf);
		}

		if (!p)
		{
			uci_free_context(state->uci);
			free(state);

			warn("Failed to load %s", STATEFILE);

			return false;
		}

		state->statefile = true;

		run_state = state;
	}
	else
	{
		if (uci_load(state->uci, "ipset", &p))
		{
			uci_perror(state->uci, NULL);
			error("Failed to load /etc/config/ipset");
		}

		if (!find_command("ipset"))
		{
			error("Unable to locate ipset utility, disabling ipset support");
		}

		cfg_state = state;
	}

	state->package = p;

	load_ipsets(state);
	load_ipset_entries(state);

	return true;
}

static void
free_state(struct state *state)
{
	struct list_head *cur, *tmp;

	if (!state)
		return;

	list_for_each_safe(cur, tmp, &state->ipsets)
	{
		free_ipset((struct ipset *)cur);
	}

	uci_free_context(state->uci);

	free(state);
}

static int
start(void)
{
	/* Try to destroy all of the previous entries from the state file
	 */
	if (run_state)
	{
		update_state(run_state);
		destroy_ipsets(run_state);
	}

	if (!cfg_state)
	{
		warn("noconfig file");
		write_statefile(run_state);
		return -1;
	}

	create_ipsets(cfg_state);
	create_ipset_entries(cfg_state);

	merge_state(cfg_state, run_state);
	write_statefile(cfg_state);

	return 0;
}

static int
stop(void)
{
	if (!run_state)
		return -1;

	flush_ipset_entries(run_state);
	destroy_ipsets(run_state);

	if (update_state(run_state) == 0)
		unlink(STATEFILE);
	else
		write_statefile(run_state);

	return 0;
}

static int
usage(const char* appname)
{
	fprintf(stderr, "%s [-dqh] {start|stop|reload}\n"
			"Options:\n"
			"\t-d\tPrint debug information to stderr\n"
			"\t-q\tSuppress stderr output\n"
			"\t-h\tShow this help\n\n", appname);

	return 1;
}

int
main(int argc, char* argv[])
{
	int ch;
	int rv = 1;

	while ((ch = getopt(argc, argv, "dqh")) != -1)
	{
		switch(ch)
		{
		case 'd':
			pr_debug = true;
			break;
		case 'q':
			freopen("/dev/null", "w", stderr);
			break;
		case 'h':
		default: // fall through
			rv = usage(argv[0]);
			goto out;
			break;
		}
	}

	build_state(false);

	if (optind >= argc)
	{
		rv = usage(argv[0]);
		goto out;
	}

	if (!strcmp(argv[optind], "start"))
	{
		if (lock())
		{
			build_state(true);
			rv = start();
			unlock();
		}
	}
	else if (!strcmp(argv[optind], "stop"))
	{
		if (lock())
		{
			build_state(true);
			rv = stop();
			unlock();
		}
	}
	else if (!strcmp(argv[optind], "reload"))
	{
		if (lock())
		{
			build_state(true);
			rv = start();
			unlock();
		}
	}
	else
	{
		rv = usage(argv[0]);
	}

out:
	if (cfg_state)
		free_state(cfg_state);

	if (run_state)
		free_state(run_state);
	return rv;
}
