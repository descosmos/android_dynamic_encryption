#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
// #include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

extern "C"{
    __attribute__((constructor(1))) void output_1() {
        printf("Hello from init_arry(1).\n");
    }
    
    __attribute__((constructor(2))) void output_2() {
        printf("Hello from init_arry(2).\n");
    }

    __attribute__((constructor(3))) void decrypt() {
        //  open
        // while (1) {
        //     sleep(2);
        //     printf(".\n");
        // }

        // int fd = open("libcaculator.so", O_RDWR);
        // if (fd == -1) {
        //     printf("Cannot open libcaculator.so");
        //     return;
        // }
        // size_t file_size_ = lseek(fd, 0, SEEK_END);

        // //  mmap
        // void *mmap_start = mmap(nullptr, file_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        // uint8_t *mmap_program_ = (uint8_t*)mmap_start;
        // if (!mmap_program_) {
        //     printf("Cannot mmap libcaculator.so");
        //     return;
        // }

        // //  get elf header
        // Elf64_Ehdr *file_header;
        // file_header = reinterpret_cast<Elf64_Ehdr*>(mmap_program_);

        // if (file_header->e_ident[EI_MAG0] != ELFMAG0 || file_header->e_ident[EI_MAG1] != ELFMAG1
        //         || file_header->e_ident[EI_MAG2] != ELFMAG2 || file_header->e_ident[EI_MAG3] != ELFMAG3) {
        //     printf("Unlegal elf header.\n");
        // }

        // printf("getpid %ld\n", getpid());
        
        // munmap(mmap_program_, file_size_);
    }


    volatile int add(int a, int b) {
        // printf("caculator: add\n");
        return a + b;
    }

    volatile int sub(int a, int b) {
        // printf("caculator: sub\n");
        return a - b;
    }

    volatile int mul(int a, int b) {
        // printf("caculator: mul\n");
        return a * b;
    }

    volatile int div(int a, int b) {
        // printf("caculator: div\n");
        return a / b;
    }
}