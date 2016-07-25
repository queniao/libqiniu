#include <stdio.h>
#include "qiniu/base/misc.h"
#include "qiniu/auth.h"

int main(int argc, char * argv[])
{
    qn_mac_ptr mac;
    qn_string url = NULL;
    qn_string download_url = NULL;

    if (argc < 4) {
        printf("Usage: qdnurl <ACCESS_KEY> <SECRET_KEY> <URL>\n");
        return 0;
    }

    mac = qn_mac_create(argv[1], argv[2]);
    url = argv[3];

    download_url = qn_mac_make_dnurl(mac, url, qn_misc_localtime() + 3600);
    if (!download_url) {
        qn_mac_destroy(mac);
        printf("Cannot make a download url for '%s'\n", url);
        return 1;
    }

    printf("%s\n", download_url);
    qn_str_destroy(download_url);
    qn_mac_destroy(mac);
    return 0;
}
