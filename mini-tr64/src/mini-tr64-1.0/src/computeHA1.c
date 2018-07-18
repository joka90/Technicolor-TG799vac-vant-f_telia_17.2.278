#include <stdio.h>
#include <string.h>
#include <openssl/md5.h>
#include "upnphttp.h"
#include "upnphttpdigest.h"

#define USERNAME_MAX_LEN 128
#define PASSWORD_MAX_LEN 128
#define REALM_MAX_LEN 128

int main(int argc, char ** argv) {
    int i;
    const char *refHA1 = "939e7578ed9e3c518a452acee763bce9";
    char compHA1[2* MD5_DIGEST_LENGTH+1];
    char username[USERNAME_MAX_LEN] = "";
    char password[PASSWORD_MAX_LEN] = "";
    char realm[REALM_MAX_LEN] = "minitr064d";
    char digest[2*MD5_DIGEST_LENGTH+1];
    int selftest;

    /* command line arguments processing */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
        }
        else
            switch (argv[i][1]) {
                case 'u':
                    if (i + 1 < argc)
                        strncpy(username, argv[++i], USERNAME_MAX_LEN);
                    else
                        fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                    username[USERNAME_MAX_LEN - 1] = '\0';
                    break;
                case 'p':
                    if (i + 1 < argc)
                        strncpy(password, argv[++i], PASSWORD_MAX_LEN);
                    else
                        fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                    password[PASSWORD_MAX_LEN - 1] = '\0';
                    break;
                case 'r':
                    if (i + 1 < argc)
                        strncpy(realm, argv[++i], REALM_MAX_LEN);
                    else
                        fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                    username[REALM_MAX_LEN - 1] = '\0';
                    break;
                default:
                    fprintf(stderr, "Unknown option: %s\n", argv[i]);
            }
    }
    if (!username[0] || !password[0] || !realm[0]) {
        /* bad configuration */
        fprintf(stderr, "Usage:\n\t"
                "%s "
                "\n"
                "\t-u username -p password -r realm\n"
                "\t-h prints this help and quits.\n"
                "", argv[0]);
        return -1;
    }


    // Using data from http://tools.ietf.org/html/rfc2617
    computeHA1("Mufasa", "Circle Of Life", "testrealm@host.com", compHA1);
    if(strcmp(refHA1, compHA1)) {
        fprintf(stderr, "Self test failed - HA1 computation not reliable\n");
    } else {
        fprintf(stderr, "Self test passed - HA1 computation reliable\n");
    }
    selftest = checkAuthentication(refHA1, "dcd98b7102dd2f0e8b11d0f600bfb0c093",
            "GET", "/dir/index.html", "6629fae49393a05397450978507c4ef1", "auth", "00000001", "0a4f113b");
    if(selftest) {
        fprintf(stderr, "Self test failed - authentication check not reliable\n");
    } else {
        fprintf(stderr, "Self test passed - authentication check reliable\n");
    }

    fprintf(stderr, "\nComputing hash for %s:%s:%s\n", username, realm, password);
    computeHA1(username, password, realm, digest);
    printf("%s\n", digest);

    return 0;
}

