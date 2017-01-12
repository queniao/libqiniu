#include <stdio.h>
#include "qiniu/base/errors.h"
#include "qiniu/base/misc.h"
#include "qiniu/cdn.h"

int main(int argc, char * argv[])
{
    char * key = argv[1];
    char * url = argv[2];
    char * deadline_str = argv[3];
    qn_uint32 deadline;
    qn_string dnurl;

    if (argc < 3) {
        printf("Demo qcdnurl - Generate a CDN download url with an authorized token (based on deadline).\n");
        printf("Usage: qcdnurl <KEY> <URL> [DEADLINE]\n");
        return 0;
    } // if

    if (deadline_str) {
        deadline = atol(deadline_str);
    } else {
        deadline = qn_misc_localtime() + 3600;
    } // if

    dnurl = qn_cdn_make_dnurl_with_deadline(key, url, deadline);
    if (! dnurl) {
        printf("Cannot make a download url with deadline for CDN due to application error '%s'\n", qn_err_get_message());
        return 1;
    } // if

    printf("%s\n", qn_str_cstr(dnurl));
    qn_str_destroy(dnurl);
    return 0;
}
