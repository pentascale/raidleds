//#define DEBUG

#include <stdio.h>      /* Standard input/output definitions */
#include <stdlib.h>     /* POSIX standard lib */
#include <string.h>     /* String function definitions */
#include <unistd.h>     /* UNIX standard function definitions */
#include <errno.h>      /* Error number definitions */

#include <fcntl.h>      /* For O_* constants */
#include <sys/stat.h>   /* For mode constants */
#include <mqueue.h>

#include "serialport.h" /* Serial port functions */
#include "sn74lv8153.h" /* Protocol functions */
#include "lshwparser.h"

#define QUEUE_MAX_MSG 1024
#define QUEUE_MSG_SIZE 4
#define QUEUE_NAME "/raidleds"
#define MDSTAT "/proc/mdstat"

int running = 1;

int set_leds(unsigned char data) {
    
    char errMsg[256];
    unsigned char packet[2];
    if (!create_packet(packet, data, DEFAULT_BANK)) {
        sprintf(errMsg, "Failed to create packet with data %X for bank %X", data, DEFAULT_BANK);
        printf("%s\n", errMsg);
	return 0;
    }

    write_port(packet, 2);

    return 1;
}

mqd_t open_queue() {
    struct mq_attr attr;
    attr.mq_maxmsg = QUEUE_MAX_MSG;
    attr.mq_msgsize = QUEUE_MSG_SIZE;
    mqd_t queue = mq_open(QUEUE_NAME, O_RDWR | O_CREAT, 0666, &attr);

    if (queue == -1) {
        char errMsg[256];
        sprintf(errMsg, "Unable to open queue - ");
        perror(errMsg);
    }

    return queue;
}

int process_queue() {

    mqd_t queue = open_queue();

    if (queue == -1) {
	return EXIT_FAILURE;
    }

    while (running) {
	char msg[QUEUE_MSG_SIZE];
	unsigned int priority;
        ssize_t received = mq_receive(queue, msg, QUEUE_MSG_SIZE, &priority);

	if (received == QUEUE_MSG_SIZE) {
            // got potentially valid message
	    if (memcmp(msg, "QUIT", 4) == 0) {
                running = 0;
	    }
        }
    }

    if (mq_close(queue)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void _pubsub(int argc, char *argv[]) {
    if (argc == 1) {
        process_queue();
    }
    else {
        char msg[] = "....";
        for (int i = 0; i < 4; i++) msg[i] = argv[1][i];
        printf("sending %c %c %c %c\n", msg[0], msg[1], msg[2], msg[3]);
        mqd_t queue = open_queue();
        if (queue != -1) {
            int sent = mq_send(queue, msg, 4, 0);
            mq_close(queue);
        }
    }
}

unsigned char read_mdstat() {
    FILE* f = fopen(MDSTAT, "r");
    if (!f) {
        return 0;
    }
    unsigned char result = 0;
    char buffer[1024];
    while (fgets(buffer, 1024, f)) {
        char* start = strstr(buffer, "level 5,");
        if (start) {
            // we're in the right line
            start = strstr(start, "] [");
            if (start) {
                // got the drive health indicators
                start += 3;
                char* stop = strstr(start, "]");
                if (stop && (stop - start == 4)) {
                    // exactly 4 characters between brackets
                    unsigned char blue_mask = 0x01;
                    unsigned char red_mask = 0x10;
                    // scan the indicators and render bits
                    for (char* c = start; c < stop; c++) {
                        if (*c == 'U') {
                            result |= blue_mask;
                        }
                        else {
                            result |= red_mask;
                        }
                        blue_mask <<= 1;
                        red_mask <<= 1;
                    }
                }
            }
        }
    }
    fclose(f);

    return result;
}

int main(int argc, char *argv[]) {

//    char drives[4][16];
//    for (int i = 0; i < 4; i++) {
//        memset(drives[i], 0, 16);
//    }
//    read_lshw(drives);
//    return 0;

    char* dev = "/dev/ttyS0";
    char errMsg[256];

    if (!open_port(dev))
    {
        return EXIT_FAILURE;
    }

    unsigned char leds = 0;

    if (argc == 3
        && !strcmp(argv[1], "-d")
        && strlen(argv[2]) == 8) {

        // process manual bit settings argument
        char* bits = argv[2];
        unsigned char mask = 0x80;
        for (int i = 0; i < 8; i++) {
            if (bits[i] != '0') {
                leds |= mask;
            }
            mask >>= 1;
        }
#ifdef DEBUG
        print_byte_binary(leds);
#endif
    }
    else {
        leds = read_mdstat();
    }

    set_leds(leds);
/*
    unsigned char data = 0x01;

    for (int i = 0; i < 8; i++) {

	if (!set_leds(data)) {
            continue;
	}

        usleep(300000);
        data <<= 1;
    }

    set_leds(0);
*/
    close_port();

    return EXIT_SUCCESS;
}
