/************** COPYRIGHT AND CONFIDENTIALITY INFORMATION ********************
**                                                                          **
** Copyright (c) 2014 Technicolor                                           **
** All Rights Reserved                                                      **
**                                                                          **
** This program contains proprietary information which is a trade           **
** secret of TECHNICOLOR and/or its affiliates and also is protected as     **
** an unpublished work under applicable Copyright laws. Recipient is        **
** to retain this program in confidence and is not permitted to use or      **
** make copies thereof other than as permitted in a written agreement       **
** with TECHNICOLOR, UNLESS OTHERWISE EXPRESSLY ALLOWED BY APPLICABLE LAWS. **
**                                                                          **
******************************************************************************/

/**
  * @file miniupnpdpath.c
  * @brief implementation of the miniupnpdpath.h functions / vars
  */

#include "minitr064dpath.h"
#include "token.h"

#define PATH_SIZE 50

/**
  * Char arrays used as the placeholders for the paths
  */
char ROOTDESC_PATH[PATH_SIZE] = ROOTDESC_PATH_END;
char BASE_CTL_PATH[PATH_SIZE] = BASE_CTL_PATH_END;

char DI_PATH[PATH_SIZE] = DI_PATH_END;
char DI_CONTROLURL[PATH_SIZE] = DI_CONTROLURL_END;

char DC_PATH[PATH_SIZE] = DC_PATH_END;
char DC_CONTROLURL[PATH_SIZE] = DC_CONTROLURL_END;

char LCS_PATH[PATH_SIZE] = LCS_PATH_END;
char LCS_CONTROLURL[PATH_SIZE] = LCS_CONTROLURL_END;

char LHC_PATH[PATH_SIZE] = LHC_PATH_END;
char LHC_CONTROLURL[PATH_SIZE] = LHC_CONTROLURL_END;
char LHC_PATTERN[PATH_SIZE] = LHC_PATTERN_END;

char LEIC_PATH[PATH_SIZE] = LEIC_PATH_END;
char LEIC_CONTROLURL[PATH_SIZE] = LEIC_CONTROLURL_END;
char LEIC_PATTERN[PATH_SIZE] = LEIC_PATTERN_END;

char WLANC_PATH[PATH_SIZE] = WLANC_PATH_END;
char WLANC_CONTROLURL[PATH_SIZE] = WLANC_CONTROLURL_END;
char WLANC_PATTERN[PATH_SIZE] = WLANC_PATTERN_END;

char WCIC_PATH[PATH_SIZE] = WCIC_PATH_END;
char WCIC_CONTROLURL[PATH_SIZE] = WCIC_CONTROLURL_END;
char WCIC_PATTERN[PATH_SIZE] = WCIC_PATTERN_END;

char WDIC_PATH[PATH_SIZE] = WDIC_PATH_END;
char WDIC_CONTROLURL[PATH_SIZE] = WDIC_CONTROLURL_END;
char WDIC_PATTERN[PATH_SIZE] = WDIC_PATTERN_END;

char WEIC_PATH[PATH_SIZE] = WEIC_PATH_END;
char WEIC_CONTROLURL[PATH_SIZE] = WEIC_CONTROLURL_END;
char WEIC_PATTERN[PATH_SIZE] = WEIC_PATTERN_END;

char WDLC_PATH[PATH_SIZE] = WDLC_PATH_END;
char WDLC_CONTROLURL[PATH_SIZE] = WDLC_CONTROLURL_END;
char WDLC_PATTERN[PATH_SIZE] = WDLC_PATTERN_END;

char WELC_PATH[PATH_SIZE] = WELC_PATH_END;
char WELC_CONTROLURL[PATH_SIZE] = WELC_CONTROLURL_END;
char WELC_PATTERN[PATH_SIZE] = WELC_PATTERN_END;

char WANPPP_PATH[PATH_SIZE] = WANPPP_PATH_END;
char WANPPP_CONTROLURL[PATH_SIZE] = WANPPP_CONTROLURL_END;
char WANPPP_PATTERN[PATH_SIZE] = WANPPP_PATTERN_END;

char WANIP_PATH[PATH_SIZE] = WANIP_PATH_END;
char WANIP_CONTROLURL[PATH_SIZE] = WANIP_CONTROLURL_END;
char WANIP_PATTERN[PATH_SIZE] = WANIP_PATTERN_END;


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

    sprintf( ROOTDESC_PATH, "/%s%s", _tokens, ROOTDESC_PATH_END );
    sprintf( BASE_CTL_PATH, "/%s%s", _tokens, BASE_CTL_PATH_END );

    sprintf( DI_PATH, "/%s%s", _tokens, DI_PATH_END );
    sprintf( DI_CONTROLURL, "/%s%s", _tokens, DI_CONTROLURL_END );

    sprintf( DC_PATH, "/%s%s", _tokens, DC_PATH_END );
    sprintf( DC_CONTROLURL, "/%s%s", _tokens, DC_CONTROLURL_END );

    sprintf( LCS_PATH, "/%s%s", _tokens, LCS_PATH_END );
    sprintf( LCS_CONTROLURL, "/%s%s", _tokens, LCS_CONTROLURL_END );

    sprintf( LHC_PATH, "/%s%s", _tokens, LHC_PATH_END );
    sprintf( LHC_CONTROLURL, "/%s%s", _tokens, LHC_CONTROLURL_END );
    sprintf( LHC_PATTERN, "POST /%s%s", _tokens, LHC_PATTERN_END);

    sprintf( LEIC_PATH, "/%s%s", _tokens, LEIC_PATH_END );
    sprintf( LEIC_CONTROLURL, "/%s%s", _tokens, LEIC_CONTROLURL_END );
    sprintf( LEIC_PATTERN, "POST /%s%s", _tokens, LEIC_PATTERN_END);

    sprintf( WLANC_PATH, "/%s%s", _tokens, WLANC_PATH_END );
    sprintf( WLANC_CONTROLURL, "/%s%s", _tokens, WLANC_CONTROLURL_END );
    sprintf( WLANC_PATTERN, "POST /%s%s", _tokens, WLANC_PATTERN_END);

    sprintf( WCIC_PATH, "/%s%s", _tokens, WCIC_PATH_END );
    sprintf( WCIC_CONTROLURL, "/%s%s", _tokens, WCIC_CONTROLURL_END );
    sprintf( WCIC_PATTERN, "POST /%s%s", _tokens, WCIC_PATTERN_END);

    sprintf( WDIC_PATH, "/%s%s", _tokens, WDIC_PATH_END );
    sprintf( WDIC_CONTROLURL, "/%s%s", _tokens, WDIC_CONTROLURL_END );
    sprintf( WDIC_PATTERN, "POST /%s%s", _tokens, WDIC_PATTERN_END);

    sprintf( WEIC_PATH, "/%s%s", _tokens, WEIC_PATH_END );
    sprintf( WEIC_CONTROLURL, "/%s%s", _tokens, WEIC_CONTROLURL_END );
    sprintf( WEIC_PATTERN, "POST /%s%s", _tokens, WEIC_PATTERN_END);

    sprintf( WDLC_PATH, "/%s%s", _tokens, WDLC_PATH_END );
    sprintf( WDLC_CONTROLURL, "/%s%s", _tokens, WDLC_CONTROLURL_END );
    sprintf( WDLC_PATTERN, "POST /%s%s", _tokens, WDLC_PATTERN_END);

    sprintf( WELC_PATH, "/%s%s", _tokens, WELC_PATH_END );
    sprintf( WELC_CONTROLURL, "/%s%s", _tokens, WELC_CONTROLURL_END );
    sprintf( WELC_PATTERN, "POST /%s%s", _tokens, WELC_PATTERN_END);

    sprintf( WANPPP_PATH, "/%s%s", _tokens, WANPPP_PATH_END );
    sprintf( WANPPP_CONTROLURL, "/%s%s", _tokens, WANPPP_CONTROLURL_END );
    sprintf( WANPPP_PATTERN, "POST /%s%s", _tokens, WANPPP_PATTERN_END);

    sprintf( WANIP_PATH, "/%s%s", _tokens, WANIP_PATH_END );
    sprintf( WANIP_CONTROLURL, "/%s%s", _tokens, WANIP_CONTROLURL_END );
    sprintf( WANIP_PATTERN, "POST /%s%s", _tokens, WANIP_PATTERN_END);

}

/**
  * Function to get the current token
  */
char *get_current_token(void )
{
	return _tokens;
}
