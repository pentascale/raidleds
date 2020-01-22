#include <stdio.h>
#include <string.h>

#define LSHW_CMD "/root/lshw.sh"

int starts_with(char* haystack, char* needle) {
    char *h = haystack, *n = needle;
    while (*n && *h && *n == *h) {
        n++;
        h++;
    }
    return (!*n) ? 1 : 0;
}

void read_lshw(char result[4][16]) {

    char buffer[1024];
    FILE* f = popen(LSHW_CMD, "r");
    if (f) {
        int index = -1;
        int found = 0;
        int last_bus = -1;
        while (fgets(buffer, 1024, f) && found < 4) {
            // remove newline
            for (char *c = buffer; *c; c++) {
                if (*c == '\n') {
                    *c = '\0';
                    break;
                }
            }
            if (starts_with(buffer, "  *-disk")) {
                index++;
                continue;
            }
            //       bus info: scsi@
            if (starts_with(buffer, "       bus info: scsi@")) {
                int pos = 0;
                char num[8];
                while (buffer[22+pos] >= '0' && buffer[22+pos] <= '9') {
                    num[pos] = buffer[22+pos];
                    pos++;
                }
                num[pos] = '\0';
                if (strlen(num)) {
                    last_bus = atoi(num);
                }
                continue;
            }
            if (starts_with(buffer, "       logical name: ")) {
                if (last_bus >=0 && last_bus < 4) {
#ifdef DEBUG
                    printf("Found %s at bus %d\n", buffer + 21, last_bus);
#endif
                    strcpy(result[last_bus], buffer + 21);
                    found++;
                }
                continue;
            }
        }
        pclose(f);
    }
}
