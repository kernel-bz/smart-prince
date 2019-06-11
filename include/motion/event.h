/*
 *    event.h
 *
 *    Include file for event.c
 *
 *    Copyright Jeroen Vreeken, 2002
 *    This software is distributed under the GNU Public License Version 2
 *    see also the file 'COPYING'.
 *
 */
#ifndef __EVENT_H
#define __EVENT_H

#define EVENT_FILECREATE        1
#define EVENT_MOTION            2
#define EVENT_FIRSTMOTION       4
#define EVENT_ENDMOTION         8
#define EVENT_STOP              16
#define EVENT_TIMELAPSE         32
#define EVENT_TIMELAPSEEND      64
#define EVENT_WEBCAM            128
#define EVENT_IMAGE_DETECTED    256
#define EVENT_IMAGEM_DETECTED   512
#define EVENT_IMAGE_SNAPSHOT    1024
#define EVENT_IMAGE             2048
#define EVENT_IMAGEM            8192
#define EVENT_FILECLOSE         16384
#define EVENT_DEBUG             65536
#define EVENT_CRITICAL          131072
#define EVENT_AREA_DETECTED     262144
#define EVENT_CAMERA_LOST       524288

typedef void(* event_handler)(struct context *, int, unsigned char *, char *, void *, struct tm *);

/*
 *    Starting point for all events
 */

struct event_handlers {
    int type;
    event_handler handler;
};

static inline const char *imageext(struct context *cnt)
{
    return (cnt->conf.ppm==VIDEO_PALETTE_GREY || cnt->conf.ppm==VIDEO_PALETTE_RGB24)
                    ? "ppm" : "jpg";
}

static inline void event(struct context *cnt, int type, unsigned char *image, char *filename, void *eventdata, struct tm *tm)
{
    /**
    int i = -1;

    while (event_handlers[++i].handler) {
        if (type & event_handlers[i].type)
            event_handlers[i].handler(cnt, type, image, filename, eventdata, tm);
    }
    */
}


#endif /* _INCLUDE_EVENT_H_ */
