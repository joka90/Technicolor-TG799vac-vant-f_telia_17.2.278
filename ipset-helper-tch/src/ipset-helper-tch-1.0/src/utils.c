/*
 * utils.c
 *
 *  Created on: Jun 13, 2016
 *      Author: geertsn
 */


#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>

#include "utils.h"
#include "options.h"
#include "ipsets.h"

static int lock_fd = -1;
static pid_t pipe_pid = -1;
static FILE *pipe_fd = NULL;

bool pr_debug = false;


static void
warn_elem_section_name(struct uci_section *s, bool find_name)
{
	int i = 0;
	struct uci_option *o;
	struct uci_element *tmp;

	if (s->anonymous)
	{
		uci_foreach_element(&s->package->sections, tmp)
		{
			if (strcmp(uci_to_section(tmp)->type, s->type))
				continue;

			if (&s->e == tmp)
				break;

			i++;
		}

		fprintf(stderr, "@%s[%d]", s->type, i);

		if (find_name)
		{
			uci_foreach_element(&s->options, tmp)
			{
				o = uci_to_option(tmp);

				if (!strcmp(tmp->name, "name") && (o->type == UCI_TYPE_STRING))
				{
					fprintf(stderr, " (%s)", o->v.string);
					break;
				}
			}
		}
	}
	else
	{
		fprintf(stderr, "'%s'", s->e.name);
	}

	if (find_name)
		fprintf(stderr, " ");
}

void
warn_elem(struct uci_element *e, const char *format, ...)
{
	if (e->type == UCI_TYPE_SECTION)
	{
		fprintf(stderr, "Warning: Section ");
		warn_elem_section_name(uci_to_section(e), true);
	}
	else if (e->type == UCI_TYPE_OPTION)
	{
		fprintf(stderr, "Warning: Option ");
		warn_elem_section_name(uci_to_option(e)->section, false);
		fprintf(stderr, ".%s ", e->name);
	}

    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);

	fprintf(stderr, "\n");
}

void
warn(const char* format, ...)
{
	fprintf(stderr, "Warning: ");
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	fprintf(stderr, "\n");
}

void
error(const char* format, ...)
{
	fprintf(stderr, "Error: ");
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	fprintf(stderr, "\n");

	exit(1);
}

void
info(const char* format, ...)
{
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
	fprintf(stderr, "\n");
}

const char *
find_command(const char *cmd)
{
	struct stat s;
	int plen = 0, clen = strlen(cmd) + 1;
	char *search, *p;
	static char path[PATH_MAX];

	if (!stat(cmd, &s) && S_ISREG(s.st_mode))
		return cmd;

	search = getenv("PATH");

	if (!search)
		search = "/bin:/usr/bin:/sbin:/usr/sbin";

	p = search;

	do
	{
		if (*p != ':' && *p != '\0')
			continue;

		plen = p - search;

		if ((plen + clen) >= sizeof(path))
			continue;

		strncpy(path, search, plen);
		sprintf(path + plen, "/%s", cmd);

		if (!stat(path, &s) && S_ISREG(s.st_mode))
			return path;

		search = p + 1;
	}
	while (*p++);

	return NULL;
}

bool
stdout_pipe(void)
{
	pipe_fd = stdout;
	return true;
}

bool
__command_pipe(bool silent, const char *command, ...)
{
	pid_t pid;
	va_list argp;
	int pfds[2];
	int argn;
	char *arg, **args, **tmp;

	command = find_command(command);

	if (!command)
		return false;

	if (pipe(pfds))
		return false;

	argn = 2;
	args = calloc(argn, sizeof(arg));

	if (!args)
		return false;

	args[0] = (char *)command;
	args[1] = NULL;

	va_start(argp, command);

	while ((arg = va_arg(argp, char *)) != NULL)
	{
		tmp = realloc(args, ++argn * sizeof(arg));

		if (!tmp)
			break;

		args = tmp;
		args[argn-2] = arg;
		args[argn-1] = NULL;
	}

	va_end(argp);

	switch ((pid = fork()))
	{
	case -1:
		return false;

	case 0:
		dup2(pfds[0], 0);

		close(pfds[0]);
		close(pfds[1]);

		close(1);

		if (silent)
			close(2);

		/* execv returns only on error */
		execv(command, args);
		error("%s\n", strerror(errno));
		return false;

	default:
		signal(SIGPIPE, SIG_IGN);
		pipe_pid = pid;
		close(pfds[0]);
		fcntl(pfds[1], F_SETFD, fcntl(pfds[1], F_GETFD) | FD_CLOEXEC);
	}

	pipe_fd = fdopen(pfds[1], "w");
	return true;
}

void
pr(const char *fmt, ...)
{
	va_list args;

	if (pr_debug && pipe_fd != stdout)
	{
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
	}

	va_start(args, fmt);
	vfprintf(pipe_fd, fmt, args);
	va_end(args);
}

void
command_close(void)
{
	if (pipe_fd && pipe_fd != stdout)
		fclose(pipe_fd);

	if (pipe_pid > -1)
		waitpid(pipe_pid, NULL, 0);

	signal(SIGPIPE, SIG_DFL);

	pipe_fd = NULL;
	pipe_pid = -1;
}

bool
lock(void)
{
	lock_fd = open(LOCKFILE, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);

	if (lock_fd < 0)
	{
		warn("Cannot create lock file: %s: %s", LOCKFILE, strerror(errno));
		return false;
	}

	if (flock(lock_fd, LOCK_EX))
	{
		warn("Cannot acquire exclusive lock: %s", strerror(errno));
		return false;
	}

	return true;
}

void
unlock(void)
{
	if (lock_fd < 0)
		return;

	if (flock(lock_fd, LOCK_UN))
		warn("Cannot release exclusive lock: %s", strerror(errno));

	close(lock_fd);
	unlink(LOCKFILE);

	lock_fd = -1;
}


static void
write_ipset_uci(struct uci_context *ctx, struct ipset *s,
                struct uci_package *dest)
{
	struct ipset_datatype *type;

	char buf[sizeof("65535-65535\0")];

	struct uci_ptr ptr = { .p = dest };

	if (!s->enabled || s->external)
		return;

	uci_add_section(ctx, dest, "ipset", &ptr.s);

	ptr.o      = NULL;
	ptr.option = "name";
	ptr.value  = s->name;
	uci_set(ctx, &ptr);

	ptr.o      = NULL;
	ptr.option = "storage";
	ptr.value  = ipset_method_names[s->method];
	uci_set(ctx, &ptr);

	list_for_each_entry(type, &s->datatypes, list)
	{
		snprintf(buf, sizeof(buf), "%s_%s", type->dir, ipset_type_names[type->type]);
		ptr.o      = NULL;
		ptr.option = "match";
		ptr.value  = buf;
		uci_add_list(ctx, &ptr);
	}

	if (s->iprange.set)
	{
		ptr.o      = NULL;
		ptr.option = "iprange";
		ptr.value  = address_to_string(&s->iprange, false, false);
		uci_set(ctx, &ptr);
	}

	if (s->portrange.set)
	{
		snprintf(buf, sizeof(buf), "%u-%u", s->portrange.port_min, s->portrange.port_max);
		ptr.o      = NULL;
		ptr.option = "portrange";
		ptr.value  = buf;
		uci_set(ctx, &ptr);
	}
}

void
write_statefile(void *state)
{
	FILE *sf;
	struct state *s = state;
	struct ipset *i;

	struct uci_package *p;

	if (!s)
		return;

	sf = fopen(STATEFILE, "w+");

	if (!sf)
	{
		warn("Cannot create state %s: %s", STATEFILE, strerror(errno));
		return;
	}

	if ((p = uci_lookup_package(s->uci, "state")) != NULL)
		uci_unload(s->uci, p);

	uci_import(s->uci, sf, "state", NULL, true);

	if ((p = uci_lookup_package(s->uci, "state")) != NULL)
	{
		list_for_each_entry(i, &s->ipsets, list)
		{
			if (check_ipset(i))
				write_ipset_uci(s->uci, i, p);
		}

		uci_export(s->uci, sf, p, true);
		uci_unload(s->uci, p);
	}

	fsync(fileno(sf));
	fclose(sf);
}

void
free_object(void *obj, const void *opts)
{
	const struct option *ol;
	struct list_head *list, *cur, *tmp;

	for (ol = opts; ol->name; ol++)
	{
		if (!ol->elem_size)
			continue;

		list = (struct list_head *)((char *)obj + ol->offset);
		list_for_each_safe(cur, tmp, list)
		{
			list_del(cur);
			free(cur);
		}
	}

	free(obj);
}

int
netmask2bitlen(int family, void *mask)
{
	int bits;
	struct in_addr *v4;
	struct in6_addr *v6;

	if (family == FAMILY_V6)
		for (bits = 0, v6 = mask;
		     bits < 128 && ((v6->s6_addr[bits / 8] << (bits % 8)) & 128);
		     bits++);
	else
		for (bits = 0, v4 = mask;
		     bits < 32 && ((ntohl(v4->s_addr) << bits) & 0x80000000);
		     bits++);

	return bits;
}

bool
bitlen2netmask(int family, int bits, void *mask)
{
	int i;
	uint8_t rem, b;
	struct in_addr *v4;
	struct in6_addr *v6;

	if (family == FAMILY_V6)
	{
		if (bits < -128 || bits > 128)
			return false;

		v6 = mask;
		rem = abs(bits);

		for (i = 0; i < sizeof(v6->s6_addr); i++)
		{
			b = (rem > 8) ? 8 : rem;
			v6->s6_addr[i] = (uint8_t)(0xFF << (8 - b));
			rem -= b;
		}

		if (bits < 0)
			for (i = 0; i < sizeof(v6->s6_addr); i++)
				v6->s6_addr[i] = ~v6->s6_addr[i];
	}
	else
	{
		if (bits < -32 || bits > 32)
			return false;

		v4 = mask;
		v4->s_addr = bits ? htonl(~((1 << (32 - abs(bits))) - 1)) : 0;

		if (bits < 0)
			v4->s_addr = ~v4->s_addr;
	}

	return true;
}

