#ifndef MINIDL_SRC_ELF_WRITER_H
#define MINIDL_SRC_ELF_WRITER_H

#include <elf.h>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <algorithm>

#include "./utils.h"
#include "./log.h"
#include "./elf_reader.h"

namespace minidl {

template<typename ElfW_Shdr> static bool sort_shdr_by_offset(const ElfW_Shdr &o1, const ElfW_Shdr& o2) {
    return o1.sh_offset < o2.sh_offset;
}

static size_t fill_zero(FILE* fp, size_t target_pos) {
    size_t cur_pos = ftell(fp);
    size_t gap_size = target_pos - cur_pos;
    if (gap_size > 0) {
        void* tmp = malloc(gap_size);
        fwrite(tmp, gap_size, 1, fp);
        free(tmp);
    }
    return gap_size;
}

/**
 * @brief Elf file writer
 */
template<typename ElfReaderW, typename ElfW_Ehdr, typename ElfW_Phdr, typename ElfW_Shdr, typename ElfW_Dyn, typename ElfW_Sym>
class ElfWriter {
public:
    ElfWriter(ElfReaderW* reader) : reader_(reader) {}

    virtual ~ElfWriter() {
    }

    void WriteEhdr() {
        // writeEhdr
        fwrite(&reader_->header_, sizeof(ElfW_Ehdr), 1, fp_);
    }

    void WritePhdrs() {
        // writePhdrs
        for (auto &phdr : reader_->phdr_) {
            fwrite(&phdr, sizeof(ElfW_Phdr), 1, fp_);
        }
    }

    void WriteSections() {
        // writeSections
        std::vector<ElfW_Shdr> shdrs(reader_->shdr_); // clone
        std::sort(shdrs.begin(), shdrs.end(), sort_shdr_by_offset<ElfW_Shdr>); // and sort
        for (auto &shdr : shdrs) {
            void* buffer = nullptr;
            size_t buffer_size;
            reader_->ReadSectionContent(shdr, &buffer, &buffer_size);
            size_t cur_pos = ftell(fp_);
            //size_t shdr.sh_offset = (cur_pos + shdr.sh_addralign - 1) & -shdr.sh_addralign;
            size_t offset_gap = shdr.sh_offset - cur_pos;
            
            //  Encrypt
            const char* shdr_name = (const char*)reader_->shstrtab_ + shdr.sh_name;
            if (strcmp(shdr_name, ".rodata") == 0) {
                // LOGI("\n-----------------------1\n");
                // char *buf = static_cast<char*>(buffer);
                // for (size_t i = 0; i < buffer_size; ++i) {
                //     printf("%c", buf[i]);
                // }
                // LOGI("\n-----------------------1\n");
                encryptRodataSection(static_cast<char*>(buffer), buffer_size);
                // LOGI("\n-----------------------2\n");
                // buf = static_cast<char*>(buffer);
                // for (size_t i = 0; i < buffer_size; ++i) {
                //     printf("%c", buf[i]);
                // }
                // LOGI("\n-----------------------2\n");
                // printf("\n");
            }

            // LOGD("cur_pos=0x%llx, offset_gap=%lld", cur_pos, offset_gap);
            
            if (buffer != nullptr && buffer_size > 0) {
                fill_zero(fp_, shdr.sh_offset);
                // LOGD("buffer %p offset:0x%llx(0x%llx), align:0x%llx, size: 0x%llx", buffer, ftell(fp), shdr.sh_offset, shdr.sh_addralign, buffer_size);
                fwrite(buffer, buffer_size, 1, fp_);
                free(buffer);
            }
        }
    }

    void WriteShdrs() {
        // writeShdrs
        fill_zero(fp_, reader_->header_.e_shoff);
        for (auto &shdr : reader_->shdr_) {
            fwrite(&shdr, sizeof(ElfW_Shdr), 1, fp_);
        }
        
    }

    bool Write(const char* output) {
        fp_ = fopen(output, "wb");
        if (fp_ == nullptr) {
            LOGE("Cannot fopen %s.\n", output);
            return false;
        }

        WriteEhdr();
        WritePhdrs();
        WriteSections();
        WriteShdrs();
        fclose(fp_);
        return true;
    }

private:
    void encryptRodataSection(char *buffer, size_t buffer_size) {
        size_t i = 0;
        while (i < buffer_size) {
            size_t Len = strlen(buffer);
            printf("buffer: %s\n", buffer);
            // for (size_t j = 0; j < Len; j++) {
            //     printf("%c", buffer[j]);
            // }
            // printf("------\n");

            if (strcmp(reader_->name_.c_str(), buffer) != 0 && 
                strcmp("/proc/self/maps", buffer) != 0 && 
                strcmp("r+", buffer) != 0 &&
                strcmp("/data/local/tmp/libcaculator.so", buffer) != 0 &&
                strcmp("/data/local/tmp/libcaculator.ss.so", buffer) != 0 &&
                strcmp("mprotect success.\n", buffer) != 0 &&
                strcmp("-", buffer) != 0 && 
                strcmp("%s\n", buffer) != 0) {
                for (size_t j = 0; j < Len; j++) {
                    // printf("%c", buffer[j]);
                    if (buffer[j] != 0) {
                        buffer[j] ^= 0xFF;
                    }
                }
            }
            buffer += (Len + 1);

            i += (Len + 1);
        }
        
        // for (size_t i = 0; i < buffer_size; ++i) {
        //     buffer[i] = buffer[i] ^ 0xFF;
        // }
    }

private:
    ElfReaderW* reader_;
    FILE* fp_{nullptr};
}; // class ElfWriter

class ElfWriter32 : public ElfWriter<ElfReader32, Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr, Elf32_Dyn, Elf32_Sym> {
public:
    ElfWriter32(ElfReader32* reader) : ElfWriter<ElfReader32, Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr, Elf32_Dyn, Elf32_Sym>(reader) {}
};

class ElfWriter64 : public ElfWriter<ElfReader64, Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr, Elf64_Dyn, Elf64_Sym> {
public:
    ElfWriter64(ElfReader64* reader) : ElfWriter<ElfReader64, Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr, Elf64_Dyn, Elf64_Sym>(reader) {}
};

} // namespace minidl


#endif // MINIDL_SRC_ELF_WRITER_H