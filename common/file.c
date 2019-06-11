/**
 *	file name:  common/file.c
 *	author:     Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *	comments:   File I/O Module
 */

 #include <stdio.h>
 #include <fcntl.h>
 #include <stdlib.h>
 #include <errno.h>
 #include <syslog.h>
 #include <string.h>
 #include <sys/stat.h>
 #include <limits.h>

#include "common/debug.h"
#include "common/config.h"
#include "motion/motion.h"

#include "darknet.h"

/**
 * create_path
 *
 *   This function creates a whole path, like mkdir -p. Example paths:
 *      this/is/an/example/
 *      /this/is/an/example/
 *   Warning: a path *must* end with a slash!
 *
 * Parameters:
 *
 *   cnt  - current thread's context structure (for logging)
 *   path - the path to create
 *
 * Returns: 0 on success, -1 on failure
 */
int create_path(const char *path)
{
    char *start;
    mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    if (path[0] == '/')
        start = strchr(path + 1, '/');
    else
        start = strchr(path, '/');

    while (start) {
        char *buffer = strdup(path);
        buffer[start-path] = 0x00;

        if (mkdir(buffer, mode) == -1 && errno != EEXIST) {
            debug_log(LOG_ERR, 1, "Problem creating directory %s", buffer);
            free(buffer);
            return -1;
        }

        free(buffer);

        start = strchr(start + 1, '/');
    }

    return 0;
}

/**
 * myfopen
 *
 *   This function opens a file, if that failed because of an ENOENT error
 *   (which is: path does not exist), the path is created and then things are
 *   tried again. This is faster then trying to create that path over and over
 *   again. If someone removes the path after it was created, myfopen will
 *   recreate the path automatically.
 *
 * Parameters:
 *
 *   path - path to the file to open
 *   mode - open mode
 *
 * Returns: the file stream object
 */
FILE * myfopen(const char *path, const char *mode)
{
    /* first, just try to open the file */
    FILE *dummy = fopen(path, mode);

    /* could not open file... */
    if (!dummy) {
        /* path did not exist? */
        if (errno == ENOENT) {

            /* create path for file... */
            if (create_path(path) == -1)
                return NULL;

            /* and retry opening the file */
            dummy = fopen(path, mode);
            if (dummy)
                return dummy;
        }

        /* two possibilities
         * 1: there was an other error while trying to open the file for the first time
         * 2: could still not open the file after the path was created
         */
        debug_log(LOG_ERR, 1, "Error opening file %s with mode %s", path, mode);

        return NULL;
    }

    return dummy;
}


/**
 * mystrftime
 *
 *   Motion-specific variant of strftime(3) that supports additional format
 *   specifiers in the format string.
 *
 * Parameters:
 *
 *   cnt        - current thread's context structure
 *   s          - destination string
 *   max        - max number of bytes to write
 *   userformat - format string
 *   tm         - time information
 *   filename   - string containing full path of filename
 *                set this to NULL if not relevant
 *   sqltype    - Filetype as used in SQL feature, set to 0 if not relevant
 *
 * Returns: number of bytes written to the string s
 */
size_t mystrftime(struct context *cnt, char *s, size_t max, const char *userformat,
                  const struct tm *tm, const char *filename, int sqltype)
{
    char formatstring[PATH_MAX] = "";
    char tempstring[PATH_MAX] = "";
    char *format, *tempstr;
    const char *pos_userformat;

    format = formatstring;

    /* if mystrftime is called with userformat = NULL we return a zero length string */
    if (userformat == NULL) {
        *s = '\0';
        return 0;
    }

    for (pos_userformat = userformat; *pos_userformat; ++pos_userformat) {

        if (*pos_userformat == '%') {
            /* Reset 'tempstr' to point to the beginning of 'tempstring',
             * otherwise we will eat up tempstring if there are many
             * format specifiers.
             */
            tempstr = tempstring;
            tempstr[0] = '\0';
            switch (*++pos_userformat) {
                case '\0': // end of string
                    --pos_userformat;
                    break;

                case 'v': // event
                    sprintf(tempstr, "%02d", cnt->event_nr);
                    break;

                case 'q': // shots
                    sprintf(tempstr, "%02d", cnt->current_image->shot);
                    break;

                case 'D': // diffs
                    sprintf(tempstr, "%d", cnt->current_image->diffs);
                    break;

                case 'N': // noise
                    sprintf(tempstr, "%d", cnt->noise);
                    break;

                case 'i': // motion width
                    sprintf(tempstr, "%d", cnt->current_image->location.width);
                    break;

                case 'J': // motion height
                    sprintf(tempstr, "%d", cnt->current_image->location.height);
                    break;

                case 'K': // motion center x
                    sprintf(tempstr, "%d", cnt->current_image->location.x);
                    break;

                case 'L': // motion center y
                    sprintf(tempstr, "%d", cnt->current_image->location.y);
                    break;

                case 'o': // threshold
                    sprintf(tempstr, "%d", cnt->threshold);
                    break;

                case 'Q': // number of labels
                    sprintf(tempstr, "%d", cnt->current_image->total_labels);
                    break;
                case 't': // thread number
                    ///sprintf(tempstr, "%d",(int)(unsigned long)
                    ///    pthread_getspecific(tls_key_threadnr));
                    break;
                case 'C': // text_event
                    if (cnt->text_event_string && cnt->text_event_string[0])
                        snprintf(tempstr, PATH_MAX, "%s", cnt->text_event_string);
                    else
                        ++pos_userformat;
                    break;
                case 'f': // filename
                    if (filename)
                        snprintf(tempstr, PATH_MAX, "%s", filename);
                    else
                        ++pos_userformat;
                    break;
                case 'n': // sqltype
                    if (sqltype)
                        sprintf(tempstr, "%d", sqltype);
                    else
                        ++pos_userformat;
                    break;
                default: // Any other code is copied with the %-sign
                    *format++ = '%';
                    *format++ = *pos_userformat;
                    continue;
            }

            /* If a format specifier was found and used, copy the result from
             * 'tempstr' to 'format'.
             */
            if (tempstr[0]) {
                while ((*format = *tempstr++) != '\0')
                    ++format;
                continue;
            }
        }

        /* For any other character than % we just simply copy the character */
        *format++ = *pos_userformat;
    }

    *format = '\0';
    format = formatstring;

    return strftime(s, max, format, tm);
}

void file_config_load(void)
{
    char* sp;

    list *cfg = read_data_cfg(CONFIG_FNAME);    ///"../data/config.txt"
    sp = option_find_str(cfg, "name", "cm01");
    strcpy(RunCfg.name, sp);
    sp = option_find_str(cfg, "net", "10.10.0.");
    strcpy(RunCfg.net, sp);
    sp = option_find_str(cfg, "ip", "10.10.0.1");
    strcpy(RunCfg.ip, sp);

    sp = option_find_str(cfg, "target", "faces");
    strcpy(RunCfg.target, sp);
    sp = option_find_str(cfg, "type", "class");
    strcpy(RunCfg.type, sp);

    RunCfg.who = option_find_int(cfg, "who", 0);
    RunCfg.id = option_find_int(cfg, "id", 1);
    RunCfg.port = option_find_int(cfg, "port", 9001);
    RunCfg.start = option_find_int(cfg, "start", 1);
    RunCfg.step = option_find_int(cfg, "step", 4);
    RunCfg.ncnt = option_find_int(cfg, "ncnt", 4);
    RunCfg.mode = option_find_int(cfg, "mode", 1);

    RunCfg.opcode = option_find_int(cfg, "opcode", 0);
    RunCfg.mplay = option_find_int(cfg, "mplay", 0);
    RunCfg.save = option_find_int(cfg, "save", 0);
    RunCfg.acc = option_find_int(cfg, "acc", 9700);

    free_list(cfg);
}

void file_config_display(void)
{
    printf("-------------------- Running Config -----------------------\n");
    printf("who:    %s\n", RunCfg.who ? "server" : "client");
    printf("name:   %s\n", RunCfg.name);
    printf("id:     %d\n", RunCfg.id);
    printf("net:    %s\n", RunCfg.net);
    printf("ip:     %s\n", RunCfg.ip);
    printf("port:   %d\n", RunCfg.port);
    printf("start:  %d\n", RunCfg.start);
    printf("step:   %d\n", RunCfg.step);
    printf("ncnt:   %d\n", RunCfg.ncnt);
    printf("mode:   %d\n", RunCfg.mode);
    printf("target: %s\n", RunCfg.target);
    printf("type:   %s\n", RunCfg.type);
    printf("opcode: %d\n", RunCfg.opcode);
    printf("mplay:  %d\n", RunCfg.mplay);
    printf("save:   %d\n", RunCfg.save);
    printf("acc:    %2.2f\n", (float)RunCfg.acc/100);
    printf("---------------------------------------------------------\n\n");
}
