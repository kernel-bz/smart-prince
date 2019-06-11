/**
 *  file name:  utils.c
 *  function:   utility module
 *  author:     JungJaeJoon(rgbi3307@nate.com) on the www.kernel.bz
 *  Copyright:  2016 www.kernel.bz
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include "common/types.h"

int util_input_string(char *prompt, char *buf)
{
    int c, i=0;

    printf("%s", prompt);
    do {
        c = getchar();
        buf[i++] = c;
    } while(c != '\n');

    buf[i-1] = '\0';

    return i;
}

int util_stack_size(void)
{
    struct rlimit   rlim;
    const rlim_t stack_size = 240 * 1024 * 1024;    ///240MB(default:8MB)
    int ret;

    ret = getrlimit(RLIMIT_STACK, &rlim);
    if (ret == 0) {
        ///default 8MB
        printf("rlim_cur=%ul, rlim_max=%ul\n", (u32)rlim.rlim_cur, (u32)rlim.rlim_max);
        if (rlim.rlim_cur < stack_size) {
            ///# ulimit -s 163840
            rlim.rlim_cur = stack_size;
            ret = setrlimit(RLIMIT_STACK, &rlim);
            if (ret) {
                printf("setrlimit(%ul) error\n", (u32)rlim.rlim_cur);
            } else {
                printf("rlim_cur=%ul, rlim_max=%ul\n", (u32)rlim.rlim_cur, (u32)rlim.rlim_max);
            }
        }
    }
    return ret;
}

int util_get_process_id(char *pname)
{
    FILE *fp;
    char buf[80] = {0};
    int pid;

    sprintf(buf, "ps aux | grep %s | awk '{print $2}'", pname);
    fp = popen(buf, "r");

    if (fp) {
        fgets(buf, 10, fp);
        //printf("buf=%s.%d\r\n", buf, atoi(buf));
        pid = atoi(buf);

    } else {
        pid = -1;
    }

	pclose(fp);

    return pid;
}
