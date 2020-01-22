#define DEFAULT_BANK 0

void print_byte_binary(unsigned char input) {
    unsigned char mask = 0x80;
    char result[] = "00000000";
    for (int i = 0; i < 8; i++) {
        if (input & mask) result[i] = '1';
        mask >>= 1;
    }
    printf("%s\n", result);
}

void print_word_binary(unsigned short input) {
    unsigned short mask = 0x8000;
    char result[] = "00000000 00000000";
    for (int i = 0; i < 16; i++) {
        int j = (i > 7) ? i + 1 : i; // account for space
        if (input & mask) result[j] = '1';
        mask >>= 1;
    }
    printf("%s\n", result);
}

/**
 * Pack 1 byte of data into sequence of 2 bytes per protocol.
 * 1st byte: START_BIT A0-A2 D0-D3
 * 2nd byte: START_BIT A0-A2 D4-D7
 *
 * START_BIT is always set.
 */
unsigned short create_packet(unsigned char buffer[2], unsigned char data, unsigned char bank) {
    if (bank >= 8) {
        return 0x00;
    }

    // construct bits backward for RS-232 transmission
    // leave bank zeroes to be reversed later
    unsigned char byte0 = (data & 0x0F) << 4 | (0x00 << 1) | 0x01; /* sent as: */
    unsigned char byte1 = (data & 0xF0)      | (0x00 << 1) | 0x01; /* sent as: */

    // reverse bank bits
    unsigned char reverse_bank = (bank & 1) << 3 | (bank & 2) << 2 | (bank & 4) << 1;

    byte0 |= reverse_bank;
    byte1 |= reverse_bank;

    unsigned short result = (byte0 << 8) | byte1;
    buffer[0] = byte0; 
    buffer[1] = byte1;

#ifdef DEBUG
    printf("byte0: ");
    print_byte_binary(byte0);
    printf("byte1: ");
    print_byte_binary(byte1);
    printf("buffer0: ");
    print_byte_binary(buffer[0]);
    printf("buffer1: ");
    print_byte_binary(buffer[1]);
    printf("result: ");
    print_word_binary(result);

    printf("\n");
#endif

    return result;
}

