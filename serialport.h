#include <stdio.h>      /* Standard input/output definitions */
#include <fcntl.h>      /* File control definitions */
#include <termios.h>    /* POSIX terminal control definitions */

#define SP_OK 1
#define SP_FAILED 0

int fd = -1; /* File descriptor for the port */

int open_port(const char* dev) {
    char errMsg[256];

    fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY); /* Must open RW though only used to write */
    if (fd == -1)
    {
       /*
        * Could not open the port.
        */
        sprintf(errMsg, "Unable to open %s - ", dev);
        perror(errMsg);
        return SP_FAILED;
    }

    struct termios options;
    tcgetattr(fd, &options);
    //memset(&options, 0, sizeof options);
    
    /* Set baud rate */
    cfsetospeed(&options, B9600);
    
    /* Set 8N1 */
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    
    /* Disable hw control */
    options.c_cflag &= ~CRTSCTS;
    
    /* Set raw output mode */
    // options.c_oflag &= ~OPOST;
    cfmakeraw(&options); /* More robust approach thanc_oflag &- ~OPOST */

    return SP_OK;
}

int close_port() {
    if (fd == -1) {
        return SP_FAILED;
    }

    int error = close(fd);
    if (!error) {
        fd = -1;
	return SP_OK;
    }

    return SP_FAILED;
}

int write_port(const void *data, int size) {

    int written = write(fd, data, size);
    return (written == size) ? SP_OK : SP_FAILED;
}
