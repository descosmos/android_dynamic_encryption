#include <stdio.h>
#include <elf.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string>

#include "../include/log.h"
#include "../include/elf_reader.h"
#include "../include/elf_write.h"

static long get_file_size(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}


bool encrypt(int fd) {
    Elf64_Ehdr *file_header = nullptr;
    if (read(fd, reinterpret_cast<char*>(file_header), sizeof(Elf64_Ehdr)) == -1) {
        LOGI("Cannot read Elf64_Ehdr from so.\n");
        return false;
    }

    return true;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        LOGE("invalid input parameters: %d.\n", argc);
        return -1;
    }
    
    char *so_name_ = argv[1];

    char current_dir[250];
    getcwd(current_dir, 250);
    printf("current_dir=%s\n", current_dir);

    const char* filename = argv[1];
    const char* output_filename = "./libcaculator.ss.so";
    size_t origin_file_size = get_file_size(filename);

    minidl::ElfReader32 reader32;
    bool success = reader32.Read(filename);

    Elf32_Shdr* ro_data = reader32.FindShdrByName(".rodata");
    if (ro_data == nullptr) {
        LOGE("Cannot find section /%s/\n", ".rodata");
        return -1;
    }
    LOGI("Success to locate .rodata.\n");

    minidl::ElfWriter32 writer32(&reader32);
    success = writer32.Write(output_filename);

    size_t new_file_size = get_file_size(filename);
    if (new_file_size == get_file_size(filename)) {
        LOGI("Success to write so.\n");
    }


    return 0;
}
