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

#include <openssl/md5.h>
#include "upnphttpdigest.h"
#include "upnpglobalvars.h"
#include "syslog.h"

#define MAXELEM_LEN (128)

void computeHA1(const char* username, const char* password, const char* realm, char *digest) {
    MD5_CTX mdContext;
    unsigned char baHA1[MD5_DIGEST_LENGTH];
    int i;


    MD5_Init (&mdContext);
    MD5_Update (&mdContext, username, strlen(username));
    MD5_Update (&mdContext, ":", 1);
    MD5_Update (&mdContext, realm, strlen(realm));
    MD5_Update (&mdContext, ":", 1);
    MD5_Update (&mdContext, password, strlen(password));
    MD5_Final (baHA1,&mdContext);
    for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&digest[2*i], "%02x", baHA1[i]);
    }
    return;
}

void computeNonce(struct upnphttp* h, long uptime, char* buffer) {
    /* Process:
        1 - (done at start of daemon) generate random secret that will be used for hash (16 bytes long)
        2 - retrieve uptime in ns (long) from monotonic clock
        3 - compute digest of uptime:clientip:secret
        4 - concatenate uptime with digest
        5 - convert to HEX string
     */
    MD5_CTX mdContext;
    unsigned char digest[MD5_DIGEST_LENGTH];
    unsigned char* pIP;
    unsigned char* pUptime = (unsigned char*) &uptime;
    int pIPLen;
    unsigned int i;

#ifdef ENABLE_IPV6
    if(h->ipv6) {
        pIP = (unsigned char*) &(h->clientaddr_v6.__in6_u);
        pIPLen = 16;
    } else {
#endif
        pIP = (unsigned char*) &(h->clientaddr.s_addr);
        pIPLen = 4;
#ifdef ENABLE_IPV6
    }
#endif

    MD5_Init (&mdContext);
    MD5_Update (&mdContext, &uptime, sizeof(uptime));
    MD5_Update (&mdContext, ":", 1);
    MD5_Update (&mdContext, pIP, pIPLen);
    MD5_Update (&mdContext, ":", 1);
    MD5_Update (&mdContext, digest_nonce_secret, sizeof(digest_nonce_secret));
    MD5_Final (digest,&mdContext);


    for(i = 0; i < sizeof(long); i++) {
        sprintf(&buffer[2*i], "%02x", pUptime[i]);
    }
    for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&buffer[2*sizeof(long) + 2*i], "%02x", digest[i]);
    }
}

int checkNonce(struct upnphttp* h, const char *nonce, long *timestamp) {
    /* Process:
        1 - extract uptime from nonce
        2 - compute digest of uptime:clientip:secret
        3 - convert to HEX string
        4 - compare with nonce contained digest
     */

    char expectedNonce[2*sizeof(long) + 2*MD5_DIGEST_LENGTH + 1] = "";
    const char *cNonceUptime;
    const char *cNoncedigest;
    long uptime;
    unsigned char *pbUptime = (unsigned char *) &uptime;
    unsigned int count;

    if(strlen(nonce) != 2 * (sizeof(long) + MD5_DIGEST_LENGTH)) {
        syslog(LOG_ERR, "Received nonce with invalid size");
        return -1;
    }

    cNonceUptime = nonce;
    cNoncedigest = &nonce[2*sizeof(long)];

    for(count = 0; count < sizeof(long); count++) {
        sscanf(cNonceUptime, "%2hhx", &pbUptime[count]);
        cNonceUptime += 2;
    }

    computeNonce(h, uptime, expectedNonce);
    syslog(LOG_DEBUG, "Expected nonce digest: %s, received: %s", expectedNonce + 2 * sizeof(long), cNoncedigest);

    if(memcmp(expectedNonce + 2*sizeof(long), cNoncedigest, 2*MD5_DIGEST_LENGTH)) {
        return -1;
    } else {
        *timestamp = uptime;
        return 0;
    }
}

void generateNonce(struct upnphttp* h, char* buffer) {

    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    computeNonce(h, time.tv_sec, buffer);
}

int buildWWWAuthenticateHeader(struct upnphttp* h) {
    char nonce[2*sizeof(long) + 2*MD5_DIGEST_LENGTH + 1] = "";

    generateNonce(h, nonce);
    syslog(LOG_DEBUG,"Sending nonce %s", nonce);
    return snprintf(h->res_buf + h->res_buflen, h->res_buf_alloclen - h->res_buflen,
                "WWW-Authenticate: Digest realm=\"minitr064d\",qop=\"auth\",nonce=\"%s\",stale=%s\r\n",
                nonce, h->authState == Stale ? "true" : "false");
}

int checkAuthentication(const char *HA1, const char *nonce, const char *method,
        const char *uri, const char *response, const char *qop, const char *nc, const char *cnonce) {

    /* Process:
        1 - Compute HA2 = MD5(method:digestURI)
        2 - Convert HA2 to hex string
        3 - if qop is auth, response = MD5(HA1:nonce:nonceCount:clientNonce:qop:HA2)
            if no qop, response = MD5(HA1:nonce:HA2)
        4 - check that response is the same in both cases
     */

    int i;
    unsigned char baHA2[MD5_DIGEST_LENGTH];
    char HA2[2* MD5_DIGEST_LENGTH+1];
    unsigned char baResponse[MD5_DIGEST_LENGTH];
    char expectedResponse[2* MD5_DIGEST_LENGTH + 1] = "";
    MD5_CTX mdContext;

    if(strlen(HA1) != 2* MD5_DIGEST_LENGTH) {
        syslog(LOG_ERR, "Invalid stored password, length is not compatible with an MD5 hash");
        return -1;
    }


    /* 1 - Compute HA2 */
    MD5_Init (&mdContext);
    MD5_Update (&mdContext, method, strlen(method));
    MD5_Update (&mdContext, ":", 1);
    MD5_Update (&mdContext, uri, strlen(uri));
    MD5_Final (baHA2,&mdContext);
    /* 2 - Convert HA2 to HEX string */
    for(i = 0; i< MD5_DIGEST_LENGTH; i++)
    {
        sprintf(&HA2[2*i], "%02x", baHA2[i]);
    }

    /* 3 - Compute response */
    MD5_Init (&mdContext);
    MD5_Update (&mdContext, HA1, 2*MD5_DIGEST_LENGTH);
    MD5_Update (&mdContext, ":", 1);
    MD5_Update (&mdContext, nonce, strlen(nonce));
    MD5_Update (&mdContext, ":", 1);
    if(strcmp("auth", qop) == 0) {
        MD5_Update (&mdContext, nc, strlen(nc));
        MD5_Update (&mdContext, ":", 1);
        MD5_Update (&mdContext, cnonce, strlen(cnonce));
        MD5_Update (&mdContext, ":", 1);
        MD5_Update (&mdContext, qop, strlen(qop));
        MD5_Update (&mdContext, ":", 1);
    }
    MD5_Update (&mdContext, HA2, 2*MD5_DIGEST_LENGTH);
    MD5_Final (baResponse,&mdContext);

    /* 4 - Convert response digest to HEX ASCII */
    for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&expectedResponse[2*i], "%02x", baResponse[i]);
    }
    expectedResponse[2* MD5_DIGEST_LENGTH] = '\0';

    syslog(LOG_DEBUG, "Expected digest %s, received digest %s", expectedResponse, response);
    if(strcmp(expectedResponse, response) == 0) {
        return 0;
    } else {
        return -1;
    }
}


void processAuthorizationHeader(struct upnphttp* h, char *header) {

    /* Authorization:Digest username="132465", realm="minitr064d", nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
    uri="/ctl/WANPPP/1/1/1", response="fe49b04af2e3f0fc6d7b8b6ac07e55bd",
    opaque="5ccc069c403ebaf9f0171e9517f40e41", qop=auth, nc=00000001, cnonce="74ea5a6f37344cdb" */

    /* Process:
        1 - check that next string after Authorization: is Digest
        2 - parse all elements (name="value",) until carriage return
     */

    char HttpCommand[16];
    char HttpUrl[128];
    char username[MAXELEM_LEN] = "";
    char realm[MAXELEM_LEN] = "";
    char nonce[MAXELEM_LEN] = "";
    char uri[MAXELEM_LEN] = "";
    char response[MAXELEM_LEN] = "";
    char opaque[MAXELEM_LEN] = "";
    char qop[MAXELEM_LEN] = "";
    char nc[MAXELEM_LEN] = "";
    char cnonce[MAXELEM_LEN] = "";
    char * HA1;
    enum authStates targetAuthState;

    char * p;
    char * endentry;
    char * startentry;
    char * equalentry;
    char * name;
    char * value;
    size_t valuelen;
    int done = 0;
    int i;
    long timestamp;
    struct timespec time;

    // Taken from upnphttp to extract METHOD and URL
    p = h->req_buf;
    for(i = 0; i<15 && *p != ' ' && *p != '\r'; i++)
        HttpCommand[i] = *(p++);
    HttpCommand[i] = '\0';
    while(*p==' ')
        p++;
    for(i = 0; i<127 && *p != ' ' && *p != '\r'; i++)
        HttpUrl[i] = *(p++);
    HttpUrl[i] = '\0';

    p = header;
    while(*p == ':' || *p == ' ' || *p == '\t')
        p++;

    if(memcmp("Digest", p, 6) != 0) {
        syslog(LOG_ERR, "Invalid authent mechanism, was expecting Digest");
        return;
    }
    p += 6;

    while(!done) {
        // Start iterating looking for name="value"
        startentry = p;
        while (*startentry == ' ' || *startentry == '\t') {
            startentry++;
        }

        endentry = startentry;
        while (*endentry != ',' && !(*endentry == '\r' && *(endentry + 1) == '\n')) {
            endentry++;
        }

        if (*endentry == '\r' && *(endentry + 1) == '\n') {
            done = 1;
        }

        equalentry = startentry;
        while (equalentry < endentry && *equalentry != '=') {
            equalentry++;
        }

        name = startentry;

        if (!(equalentry[1] == '"' && endentry[-1] == '"')
                && !(equalentry[1] == '\'' && endentry[-1] == '\'')) {
            value = equalentry + 1;
            valuelen = endentry - (equalentry + 1);
        } else {
            value = equalentry + 2;
            valuelen = (endentry - 1) - (equalentry + 2);
        }

        if (valuelen > MAXELEM_LEN + 1) {
            syslog(LOG_ERR, "Authorization field too long, aborting");
            return;
        }

        if (memcmp("username", name, 8) == 0) {
            memcpy(username, value, valuelen);
            username[valuelen] = '\0';
        } else if (memcmp("qop", name, 3) == 0) {
            memcpy(qop, value, valuelen);
            qop[valuelen] = '\0';
        } else if (memcmp("realm", name, 5) == 0) {
            memcpy(realm, value, valuelen);
            realm[valuelen] = '\0';
        } else if (memcmp("nonce", name, 5) == 0) {
            memcpy(nonce, value, valuelen);
            nonce[valuelen] = '\0';
        } else if (memcmp("uri", name, 3) == 0) {
            memcpy(uri, value, valuelen);
            uri[valuelen] = '\0';
        } else if (memcmp("response", name, 8) == 0) {
            memcpy(response, value, valuelen);
            response[valuelen] = '\0';
        } else if (memcmp("opaque", name, 6) == 0) {
            memcpy(opaque, value, valuelen);
            opaque[valuelen] = '\0';
        } else if (memcmp("nc", name, 2) == 0) {
            memcpy(nc, value, valuelen);
            nc[valuelen] = '\0';
        } else if (memcmp("cnonce", name, 6) == 0) {
            memcpy(cnonce, value, valuelen);
            cnonce[valuelen] = '\0';
        }

        p = endentry + 1;
    }

    syslog(LOG_DEBUG, "Found Digest Authorization header username=%s, realm=%s, nonce=%s, uri=%s, "
            "response=%s, opaque=%s, qop=%s, nc=%s, cnonce=%s", username, realm, nonce, uri, response, opaque, qop, nc,
            cnonce);

    if(strcmp("minitr064d", realm)) {
        syslog(LOG_ERR, "Trying to log-in with invalid realm %s", realm);
        return;
    }
    if(strcmp("dslf-config", username) && strcmp("dslf-reset", username)) {
        syslog(LOG_ERR, "Trying to log-in with invalid username %s", username);
        return;
    }
    if(qop[0] != '\0' && strcmp("auth", qop)) {
        syslog(LOG_ERR, "Trying to log-in with invalid qop %s", qop);
        return;
    }
    if(checkNonce(h, nonce, &timestamp)) {
        syslog(LOG_ERR, "Invalid nonce received");
        return;
    }
    if(strcmp(HttpUrl, uri)) {
        syslog(LOG_ERR, "Invalud URI in header %s, does not match actual URI %s", uri, HttpUrl);
        return;
    }

    syslog(LOG_DEBUG, "Method is %s", HttpCommand);

    if(strcmp("dslf-config", username) == 0) {
        HA1 = password_dsl_config;
        targetAuthState = DSLConfig;
    } else {
        HA1 = password_dsl_reset;
        targetAuthState = DSLReset;
    }

    if(checkAuthentication(HA1, nonce, HttpCommand, uri, response, qop, nc, cnonce) == 0) {
        clock_gettime(CLOCK_MONOTONIC, &time);
        if((time.tv_sec - timestamp) > digest_nonce_lifetime) {
            syslog(LOG_INFO, "Nonce is stale, requesting reauth");
            h->authState = Stale;
        } else {
            h->authState = targetAuthState;
        }
    }
    return;
}
