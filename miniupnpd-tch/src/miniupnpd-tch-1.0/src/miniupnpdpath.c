/**
 * @file miniupnpdpath.c
 * @brief implementation of the miniupnpdpath.h functions / vars
 */

#include "miniupnpdpath.h"
#include "token.h"

#define PATH_SIZE 50

/**
 * Char arrays used as the placeholders for the paths
 */
char ROOTDESC_PATH[PATH_SIZE] = ROOTDESC_PATH_END;
char WANIPC_CONTROLURL[PATH_SIZE] = WANIPC_CONTROLURL_END;
char DUMMY_PATH[PATH_SIZE] = DUMMY_PATH_END;
char WANCFG_PATH[PATH_SIZE] = WANCFG_PATH_END;
char WANCFG_CONTROLURL[PATH_SIZE] = WANCFG_CONTROLURL_END;
char WANCFG_EVENTURL[PATH_SIZE] = WANCFG_EVENTURL_END;
char WANIPC_PATH[PATH_SIZE] = WANIPC_PATH_END;
char WANIPC_EVENTURL[PATH_SIZE] = WANIPC_EVENTURL_END;
char L3F_PATH[PATH_SIZE] = L3F_PATH_END;
char L3F_CONTROLURL[PATH_SIZE] = L3F_CONTROLURL_END;
char L3F_EVENTURL[PATH_SIZE] = L3F_EVENTURL_END;
char WANIP6FC_PATH[PATH_SIZE] = WANIP6FC_PATH_END;
char WANIP6FC_CONTROLURL[PATH_SIZE] = WANIP6FC_CONTROLURL_END;
char WANIP6FC_EVENTURL[PATH_SIZE] = WANIP6FC_EVENTURL_END;
char DP_PATH[PATH_SIZE] = DP_PATH_END;
char DP_CONTROLURL[PATH_SIZE] = DP_CONTROLURL_END;
char DP_EVENTURL[PATH_SIZE] = DP_EVENTURL_END;

/**
 * The current token string for the CSFR protection
 */
static char _tokens[8]; 

/**
 * Generate the paths with the token in it
 */
void generate_paths(void)
{
	generate_token_string( _tokens, 8 );

	sprintf( WANIPC_CONTROLURL, "/%s%s", _tokens, WANIPC_CONTROLURL_END );	
	sprintf( DUMMY_PATH, "/%s%s", _tokens, DUMMY_PATH_END );
	sprintf( WANCFG_PATH, "/%s%s", _tokens, WANCFG_PATH_END );
	sprintf( WANCFG_CONTROLURL, "/%s%s", _tokens, WANCFG_CONTROLURL_END );
	sprintf( WANCFG_EVENTURL, "/%s%s", _tokens, WANCFG_EVENTURL_END );
	sprintf( WANIPC_PATH, "/%s%s", _tokens, WANIPC_PATH_END );
	sprintf( WANIPC_EVENTURL, "/%s%s", _tokens, WANIPC_EVENTURL_END );
	sprintf( L3F_PATH, "/%s%s", _tokens, L3F_PATH_END );
	sprintf( L3F_CONTROLURL, "/%s%s", _tokens, L3F_CONTROLURL_END );
	sprintf( L3F_EVENTURL, "/%s%s", _tokens, L3F_EVENTURL_END );
	sprintf( WANIP6FC_PATH, "/%s%s", _tokens, WANIP6FC_PATH_END );
	sprintf( WANIP6FC_CONTROLURL, "/%s%s", _tokens, WANIP6FC_CONTROLURL_END );
	sprintf( WANIP6FC_EVENTURL, "/%s%s", _tokens, WANIP6FC_EVENTURL_END );
	sprintf( DP_PATH, "/%s%s", _tokens, DP_PATH_END );
	sprintf( DP_CONTROLURL, "/%s%s", _tokens, DP_CONTROLURL_END );
	sprintf( DP_EVENTURL, "/%s%s", _tokens, DP_EVENTURL_END );

}

/**
 * Function to get the current token
 */
char *get_current_token(void )
{
	return _tokens;
}
