#ifndef BOARD_BUTTON_DEFINES_H__
#define BOARD_BUTTON_DEFINES_H__

/*
 * Defines the board led defines for each Technicolor board
 *
 * Copyright (C) 2013 Technicolor <linuxgw@technicolor.com>
 *
 */

struct board {
	const char *				name;
	struct gpio_keys_platform_data *			buttons;
};

/**
 * Retrieves the led description for a particular board
 */
struct board * get_board_description(const char * current_board);

#endif
