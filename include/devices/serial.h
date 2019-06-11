//
//	file name: serial.h
//	comments: Serial Port Module
//	author: Jung,JaeJoon(rgbi3307@nate.com) at the www.kernel.bz
//

#ifndef __SERIAL_H
#define __SERIAL_H

#define BUFSIZE		256
//#define BAUDRATE	B9600
//#define BAUDRATE	B38400
#define BAUDRATE	B115200

#define DEV_SERIAL_NODE	"/dev/ttyAMA0"
///#define DEV_SERIAL_NODE	"/dev/tty0"

extern int SerialDevFd;

int serial_open (char *device, int mode);
int serial_close (void);

void serial_setting(int fd);
int serial_send (char *buf);
int serial_recv (char *buf);

//wait: seconds
int serial_send_select (char *buf, int wait);
int serial_send_poll (char *buf, int wait);

//wait: seconds
int serial_read_select (int fd, char *buf, int wait);
//wait: seconds
int serial_read_poll (int fd, char *buf, int wait);

void serial_test(int motor, int dir, int speed, int duration);

#endif
