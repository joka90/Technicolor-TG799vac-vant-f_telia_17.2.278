/*
 * utils.h
 *
 *  Created on: Jun 13, 2016
 *      Author: geertsn
 */

#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#include <stdarg.h>

#include <libubox/list.h>
#include <uci.h>

#define STATEFILE	"/var/run/ipset-helper.state"
#define LOCKFILE	"/var/run/ipset-helper.lock"

extern bool pr_debug;

void warn_elem(struct uci_element *e, const char *format, ...);
void warn(const char *format, ...);
void error(const char *format, ...);
void info(const char *format, ...);

const char * find_command(const char *cmd);
bool stdout_pipe(void);
bool __command_pipe(bool silent, const char *command, ...);
#define command_pipe(...) __command_pipe(__VA_ARGS__, NULL)
void command_close(void);
void pr(const char *fmt, ...);

bool lock(void);
void unlock(void);

void write_statefile(void *state);

void free_object(void *obj, const void *opts);

int netmask2bitlen(int family, void *mask);
bool bitlen2netmask(int family, int bits, void *mask);

#endif /* SRC_UTILS_H_ */
