/**
    file name:  sp_server/main.c
    author:     JungJaeJoon(rgbi3307@nate.com)
    comments:   SmartPrince Server Main
    Copyright (c) www.kernel.bz
    This is under the GPL License (see <http://www.gnu.org/licenses/>)
 */

#include <stdio.h>
#include <stdlib.h>

#include "classifier.h"

#include "common/debug.h"
#include "motion/video.h"
#include "common/file.h"
#include "common/utils.h"
#include "common/config.h"

#include "devices/networks.h"


static void _main_thread_net_init(void)
{
    pthread_t  tid;
    if (pthread_create(&tid, NULL, net_server_thread, NULL)) {
        pr_err_msg("Can't Create Network Server Thread.\n");
    } else {
        pr_info_msg("Network Server Created: thread id=%lu [OK]\n", tid);
    }

}

static void _main_run(void)
{
    char datfile[40];   ///data config file name
    char cfgfile[40];   ///config file name
    char wtsfile[40];   ///weights file name
    unsigned int run_mode = 1 << RunCfg.mode;

    sprintf(datfile, "../%s/%s.dat", RunCfg.target, RunCfg.type);
    sprintf(cfgfile, "../%s/%s.cfg", RunCfg.target, RunCfg.type);
    sprintf(wtsfile, "../%s/%s.wts", RunCfg.target, RunCfg.type);

    if (run_mode & CONFIG_MODE_RUN)
    {
        _main_thread_net_init();

        classifier_thread_run(datfile, cfgfile, wtsfile);

        while(1) pause();

    } else  if (run_mode & CONFIG_MODE_TRAIN) {
        classifier_train_run(datfile, cfgfile, wtsfile);

    } else  if (run_mode & CONFIG_MODE_VALID) {
        classifier_validate_run(datfile, cfgfile, wtsfile);
    }
}


int main(int argc, char **argv)
{
    if (util_stack_size()) exit(-1);

    file_config_load();     ///RunCfg
    if (argc > 1) {
        if (!strcmp(argv[1], "run")) RunCfg.mode = 0;
        else if (!strcmp(argv[1], "train")) RunCfg.mode = 2;
        else if (!strcmp(argv[1], "valid")) RunCfg.mode = 3;
        else RunCfg.mode = 1;
    }
    file_config_display();

    _main_run();

    file_config_display();

    return 0;
}
