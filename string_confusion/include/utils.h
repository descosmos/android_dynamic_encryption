#ifndef MINIDL_SRC_UTILS_H
#define MINIDL_SRC_UTILS_H

#include <cstdio>
#include <cstdlib> // for malloc/free
#include <unistd.h> // for lseek/read
#include <cstring> // for memcpy
#include <algorithm>

#include "./log.h"

namespace minidl {

inline void hexdump(void *addr, int len, int offset = 0) 
{
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                PRINTF("  %s\n", buff);

            // Output the offset.
            PRINTF("  %08x ", i + offset);
        }

        // Now the hex code for the specific character.
        PRINTF(" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) {
            buff[i % 16] = '.';
        } else {
            buff[i % 16] = pc[i];
        }

        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        PRINTF("   ");
        i++;
    }

    // And print the final ASCII bit.
    PRINTF("  %s\n", buff);
}

inline size_t copy_file_into_memory(void* dest, size_t __len, int __fd, off_t __offset) {
    size_t count = 0;
    size_t file_size = lseek(__fd, 0, SEEK_END);
    size_t buffer_size = 8 * 1024;
    void* buffer = malloc(buffer_size);
    if (buffer) {
        size_t actual; 
        off_t offset = __offset;
        void* target = dest;
        while (count < __len) {
            if (offset > file_size) {
                break;
            }

            lseek(__fd, offset, SEEK_SET);
            size_t size = std::min(std::min(buffer_size, static_cast<size_t>(file_size - offset)), static_cast<size_t>(__len - count));
            actual = read(__fd, buffer, size);
            //hexdump(buffer, actual, 0);
            LOGD("read file, offset=%ld, size=%ld, actual_size=%ld", offset, size, actual);
            if (actual == 0) {
                break;
            }

            memcpy(target, buffer, actual);
            LOGD("memcpy, dest=%p, src=%p, size=%ld", target, buffer, actual);
            target = (char*)target + actual;
            offset += actual; 
            count += actual;
        }
        free(buffer);
    }
    return count;
}

} // namespace minidl

#endif // MINIDL_SRC_UTILS_H