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
        
        size_t str_len = strlen(str);

        //  修改段属性
        size_t page_start, page_end;
        ret_page_address(&page_start, &page_end);
        size_t page_size = page_end - page_start;
        char *page_adress = (char*)(page_start);

        if (mprotect(page_adress, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) == -1) {
            // printf("mprotect failed.\n");
            return;
        }
        // printf("mprotect success.\n");

        printf("before encrypted str: %s\n", str);
        for (int i = 0; i <= str_len; ++i) {
            str[i] ^= 0xff;
        }
        printf("after encrypted str: %s\n", str);

        mprotect(page_adress, page_size, PROT_READ | PROT_EXEC);
    }
    
    __attribute__((constructor)) void output_2() {
        printf("Hello from init_arry(2).\n");
    }

    size_t htoi(char *s) {
        size_t i;
        size_t n = 0;
        if (s[0] == '0' && (s[1]=='x' || s[1]=='X')) {
            i = 2;
        } else {
            i = 0;
        }
        
        for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z');++i) {
            if (tolower(s[i]) > '9') {
                n = 16 * n + (10 + tolower(s[i]) - 'a');
            } else {
                n = 16 * n + (tolower(s[i]) - '0');
            }
        }
        return n;
    }

    void ret_page_address(size_t *page_start, size_t *page_end) {
        pid_t pid = getpid();
        if (pid == -1) {
            printf("getpid failed.\n");
            return;
        }
        printf("ret_page_start\n");
        char file[30] = {0};
        sprintf(file, "/proc/%d/maps", pid);
        printf("file: %s\n", file);

        FILE *fp = fopen(file, "r+");
        if (fp == nullptr) {
            printf("fopen failed.\n");
            return;
        }

        char address[20] = {0};
        char so_name[30] = {0};
        int flags;

        //  移动文件指针，得到 libcaculator.so 的首地址
        //  TODO: /proc/pid/maps 文件中 映射文件所属节点号 对应 flags 为0 的时候会出现问题，需注意
        while (true) {
            fscanf(fp, "%s%*s%*s%*s%d", address, &flags);
            printf("----- flags: %d.\n", flags);
            if (flags != 0) {
                fscanf(fp, "%s", so_name);
                if (strcmp("/data/local/tmp/libcaculator.so", so_name) == 0) {
                    break;
                }
            }
        }

        char start[16] = {0};
        char end[16] = {0};
        printf("address: %s\n", address);
        printf("so_name: %s\n", so_name);

        //  获得so段的入口地址以及结束地址
        char *delimiter = strstr(address, "-");
        printf("delimiter: %s\n", delimiter);
        strncpy(start, address, 8);
        strncpy(end, delimiter+1, 8);
        printf("start: %s\n", start);
        printf("end: %s\n", end);

        //  将十六进制数字字符串转成无符号长整数
        *page_start = htoi(start);
        *page_end = htoi(end);
        printf("*page_start: %lu\n", *page_start);
        printf("*page_end: %lu\n", *page_end);

        // FILE *fd = fopen("/", "r+");
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