#include <stdio.h>
#include "qiniu/base/misc.h"
#include "qiniu/auth.h"

int main(int argc, char * argv[])
{
    qn_mac_ptr mac;
    qn_string url = NULL;
    qn_string acctoken = NULL;
    qn_string body = NULL;
    size_t body_size = 0;

    if (argc < 4) {
        printf("Usage: qdnurl <ACCESS_KEY> <SECRET_KEY> <URL> <BODY>\n");
        return 0;
    }

    mac = qn_mac_create(argv[1], argv[2]);
    url = argv[3];
    if (argc == 5) {
        body = argv[4];
        body_size = strlen(body);
    }

    acctoken = qn_mac_make_acctoken(mac, url, body, body_size);
    if (!acctoken) {
        qn_mac_destroy(mac);
        printf("Cannot make a acctoken for '%s'\n", url);
        return 1;
    }

    printf("%s\n", acctoken);
    qn_str_destroy(acctoken);
    qn_mac_destroy(mac);
    return 0;
}
