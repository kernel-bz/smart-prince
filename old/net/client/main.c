/**
 * Name:    sp_client/main.c
 * Purpose: SmartPrince Relay control module
 * Author:	JungJaeJoon on the www.kernel.bz
 *
 *  Copyright(C) www.kernel.bz
 *  This code is licenced under the GPL.
  *
 *  Editted:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#include <sys/stat.h>
//#include <sys/statvfs.h>
//#include <sys/wait.h>
//#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>


int get_process_id(char *arg)
{
    FILE *fp;
    char buf[80] = {0};
    int pid;

    sprintf(buf, "ps aux | grep %s | awk '{print $2}'", arg);
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


int main(int argc, char **argv)
{
    int pid, idx;
    int sig[] = { SIGUSR1, SIGUSR2, SIGALRM, SIGVTALRM,  SIGTTIN, SIGTTOU, SIGURG, SIGPROF };

    if (argc > 2) {
        idx = atoi(argv[1]);
        pid = get_process_id(argv[2]);
    } else if (argc > 1) {
        idx = atoi(argv[1]);
        pid = get_process_id("sp_server");
    } else {
        printf("Usage: ./sp_client <#>\n\n");
        return 0;
    }

    if (pid > 0 && idx >= 0 && idx < 8)
            kill(pid, sig[idx]);

    return 0;
}
