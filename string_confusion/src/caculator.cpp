#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
// #include <stdlib.h>

extern "C"{
    size_t htoi(char *s);
    void ret_page_address(size_t *page_start, size_t *page_end);
    
    __attribute__((constructor)) void output_1() {
        char *str = "Hello from init_arry(1).\n";
        // printf("..................\n");
        size_t str_len = strlen(str);

        //  修改段属性
        size_t page_start, page_end;
        ret_page_address(&page_start, &page_end);
        size_t page_size = page_end - page_start;

        // while (true) {
        //     printf("%d\n", getpid());
        //     printf("page_start: 0x%lx     page_end: 0x%lx\n", page_start, page_end);
        //     sleep(1);
        // }
        
        char *page_adress = (char*)(page_start);

        if (mprotect(page_adress, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) == -1) {
            // printf("mprotect failed.\n");
            return;
        }
        // printf("mprotect success.\n");

        size_t offset = str - page_adress;

        // printf("before encrypted str: %s\n", str);

        for (int i = 0; i <= str_len; ++i) {
            if (*(page_adress+offset+i) != 0) {
                *(page_adress + offset + i) = *(page_adress + offset + i) ^ 0xff;
            }
        }
        // memcpy((void*)str, (void*)str, str_len-1);
        // printf("after encrypted str: %s\n", str);
        printf("%s\n", str);

        mprotect(page_adress, page_size, PROT_READ | PROT_EXEC);
    }
    
    __attribute__((constructor)) void output_2() {
        printf("Hello from init_arry(2).\n");
    }


    void ret_page_address(size_t *page_start, size_t *page_end) {
        char file_path[32] = {0};
        char file_line[1024] = {0};
        FILE *fp = NULL;

        sprintf(file_path, "/proc/self/maps");

        fp = fopen(file_path, "r+");

        if (fp != NULL) {
            while (fgets(file_line, 1023, fp) != NULL) {
                if (strstr(file_line, "/data/local/tmp/libcaculator.so") || strstr(file_line, "/data/local/tmp/libcaculator.ss.so")) {
                    char *token = strtok(file_line, "-");
                    *page_start = strtoul(token, NULL, 16);

                    token = strtok(NULL, "-");
                    *page_end = strtoul(token, NULL, 16);
                    break;
                }
            }
        }

    }

    // volatile int add(int a, int b) {
    //     // printf("caculator: add\n");
    //     return a + b;
    // }

    // volatile int sub(int a, int b) {
    //     // printf("caculator: sub\n");
    //     return a - b;
    // }

    // volatile int mul(int a, int b) {
    //     // printf("caculator: mul\n");
    //     return a * b;
    // }

    // volatile int div(int a, int b) {
    //     // printf("caculator: div\n");
    //     return a / b;
    // }
}