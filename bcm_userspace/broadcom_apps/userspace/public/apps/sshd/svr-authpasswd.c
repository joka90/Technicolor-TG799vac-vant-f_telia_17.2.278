/*
 * Dropbear - a SSH2 server
 * 
 * Copyright (c) 2002,2003 Matt Johnston
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

/* Validates a user password */

#include "includes.h"
#include "session.h"
#include "buffer.h"
#include "dbutil.h"
#include "auth.h"
// BRCM begin
#ifndef SSHD_GENKEY
#ifdef BRCM_CMS_BUILD
#undef BASE64
#include "cms.h"
#include "cms_cli.h"
#include "cms_dal.h"
#include "cms_seclog.h"
extern int   glbAccessMode;
extern UINT8 currPerm;
extern char  curIpAddr[64];
#endif
#endif
// BRCM end
#ifdef ENABLE_SVR_PASSWORD_AUTH

/* Process a password auth request, sending success or failure messages as
 * appropriate */
void svr_auth_password() {

//brcm begin
#ifndef SSHD_GENKEY 	

#ifdef HAVE_SHADOW_H
	struct spwd *spasswd = NULL;
#endif
	char * passwdcrypt = NULL; /* the crypt from /etc/passwd or /etc/shadow */
	char * testcrypt = NULL; /* crypt generated from the user's password sent */
	unsigned char * password;
	unsigned int passwordlen;
#ifdef BRCM_CMS_BUILD
    CmsSecurityLogData logData = { 0 };

	// brcm add matched flag.
	int matched = 0;
#endif
	unsigned int changepw;
	passwdcrypt = ses.authstate.pw->pw_passwd;
#ifdef HAVE_SHADOW_H
	/* get the shadow password if possible */
	spasswd = getspnam(ses.authstate.printableuser);
	if (spasswd != NULL && spasswd->sp_pwdp != NULL) {
		passwdcrypt = spasswd->sp_pwdp;
	}
#endif

#ifdef DEBUG_HACKCRYPT
	/* debugging crypt for non-root testing with shadows */
	passwdcrypt = DEBUG_HACKCRYPT;
#endif

	/* check for empty password - need to do this again here
	 * since the shadow password may differ to that tested
	 * in auth.c */
	if (passwdcrypt[0] == '\0') {
		dropbear_log(LOG_WARNING, "user '%s' has blank password, rejected",
				ses.authstate.printableuser);
		send_msg_userauth_failure(0, 1);
		return;
	}

	/* check if client wants to change password */
	changepw = buf_getbool(ses.payload);
	if (changepw) {
		/* not implemented by this server */
		send_msg_userauth_failure(0, 1);
		return;
	}


	password = buf_getstring(ses.payload, &passwordlen);
	/* the first bytes of passwdcrypt are the salt */
	testcrypt = crypt((char*)password, passwdcrypt); 
#ifdef BRCM_CMS_BUILD
   // brcm add local/remote login check
   // We are doing all this auth checking inside sshd code instead of via proper CLI API.
    if ((glbAccessMode == NETWORK_ACCESS_LAN_SIDE && \
        (!strcmp(ses.authstate.username, "user") || !strcmp(ses.authstate.username, "admin"))) ||
        (glbAccessMode == NETWORK_ACCESS_WAN_SIDE && !strcmp(ses.authstate.username, "support")))
    {
        matched = 1;

        /* update cli lib with the application data */
        cmsCli_setAppData("SSHD", NULL, ses.authstate.username, SSHD_PORT);
                
        if (!strcmp(ses.authstate.username, "admin"))
        {
           currPerm = 0x80; /*PERM_ADMIN */
        }
        else if (!strcmp(ses.authstate.username, "support"))
        {
           currPerm = 0x40; /* PERM_SUPPORT */
        }
        else if (!strcmp(ses.authstate.username, "user"))
        {
           currPerm = 0x01;  /* PERM_USER */
        }
    }

	m_burn(password, passwordlen);
	m_free(password);

	CMSLOG_SEC_SET_PORT(&logData, SSHD_PORT);
	CMSLOG_SEC_SET_APP_NAME(&logData, "SSHD");
	CMSLOG_SEC_SET_USER(&logData, ses.authstate.username);
	CMSLOG_SEC_SET_SRC_IP(&logData, &curIpAddr[0]);
	if (strcmp(testcrypt, passwdcrypt) == 0 && matched) {
		/* successful authentication */
   // brcm commented next msg
		//dropbear_log(LOG_NOTICE, 
		//		"password auth succeeded for '%s' from %s",
		//		ses.authstate.printableuser,
		//		svr_ses.addrstring);
		send_msg_userauth_success();
		cmsLog_security(LOG_SECURITY_AUTH_LOGIN_PASS, &logData, NULL);
	} else {
#ifdef DESKTOP_LINUX
		dropbear_log(LOG_WARNING, "skip password auth for now, return success");
		send_msg_userauth_success();
#else
//		dropbear_log(LOG_WARNING,
//				"bad password attempt for '%s' from %s",
//				ses.authstate.printableuser,
//				svr_ses.addrstring);
//		send_msg_userauth_failure(0, 1);
#endif
		cmsLog_security(LOG_SECURITY_AUTH_LOGIN_FAIL, &logData, NULL);
	}
#endif /* BRCM_CMS_BUILD */
#endif // brcm end, ifndef SSHD_GENKEY
}

#endif
