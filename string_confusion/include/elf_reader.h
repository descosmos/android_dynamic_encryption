#ifndef MINIDL_SRC_ELF_READER_H
#define MINIDL_SRC_ELF_READER_H

#include <elf.h>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace minidl {

template<typename ElfW_Ehdr, typename ElfW_Phdr, typename ElfW_Shdr, typename ElfW_Dyn, typename ElfW_Sym>
class ElfReader {

public:
    ElfReader() : fd_(-1), shstrtab_(nullptr), shstrtab_size_(0),
                  strtab_(nullptr), strtab_size_(0),
                  dynstr_(nullptr), dynstr_size_(0),
                  file_size_(0) {}

    virtual ~ElfReader() {
        if (shstrtab_ != nullptr) {
            free(shstrtab_);
            shstrtab_ = nullptr;
        }
        if (strtab_ != nullptr) {
            free(strtab_);
            strtab_ = nullptr;
        }
        if (dynstr_ != nullptr) {
            free(dynstr_);
            dynstr_ = nullptr;
        }

        if (fd_ != 0) {
            close(fd_);
        }
    }

    /**
     * @brief Read the elf file
     * 
     * @param elf_file elf file to read
     * @return true success
     * @return false fail 
     */
    bool Read(const char* elf_file) {
        name_ = elf_file;
        fd_ = open(elf_file, O_RDONLY); // TODO: close
        file_size_ = lseek(fd_, 0, SEEK_END);
        lseek(fd_, 0, SEEK_SET);

        if (ReadElfHeader() &&
            VerifyElfHeader() &&
            ReadProgramHeaders() &&
            ReadSectionHeaders() &&
            ReadDynamicSection() &&
            ReadSymbols()) {
            return true;
        }
        return false;
    }

    /**
     * @brief Find section header by type
     * 
     * @param type section type, see SHT_*
     * @return ElfW_Shdr* 
     */
    ElfW_Shdr* FindShdrByType(uint32_t type) const {
        for (auto &shdr : shdr_) {
            if (shdr.sh_type == type) {
                return const_cast<ElfW_Shdr*>(&shdr);
            }
        }
        return nullptr;
    }

    /**
     * @brief Find first section header by name
     * 
     * @param name section name, e.g. ".strtab", ".text"
     * @return ElfW_Shdr* 
     */
    ElfW_Shdr* FindShdrByName(const char* name) const {
        for (auto &shdr : shdr_) {
            const char* shdr_name = (const char*)shstrtab_ + shdr.sh_name;
            if (strcmp(shdr_name, name) == 0) {
                return const_cast<ElfW_Shdr*>(&shdr);
            }
        }
        return nullptr;
    }

    /**
     * @brief Find first program header by type
     * 
     * @param type program type, see PT_*
     * @return ElfW_Phdr* 
     */
    ElfW_Phdr* FindPhdrByType(uint32_t type) const {
        for (auto &phdr : phdr_) {
            if (phdr.p_type == type) {
                return const_cast<ElfW_Phdr*>(&phdr); 
            }
        }
        return nullptr;
    }

    /**
     * @brief Get all program header table
     * 
     * @return std::vector<ElfW_Phdr> 
     */
    const std::vector<ElfW_Phdr>& GetAllPhdr() const {
        return phdr_;
    }

    /**
     * @brief Get size of elf file
     * 
     * @return size_t file size
     */
    size_t GetFileSize() const {
        return file_size_;
    }

    /**
     * @brief Get file descriptor of elf file
     * 
     * @return int file descriptor.
     */
    int GetFd() const {
        return fd_;
    }

    /**
     * @brief Get the Elf Header
     * 
     * @return ElfW_Ehdr 
     */
    ElfW_Ehdr GetHeader() const {
        return header_;
    }

    void ReadSectionContent(ElfW_Shdr &shdr, void** buffer, size_t* buffer_size) {
        LOGD("ReadSectionContent offset=0x%llx, size=0x%llx", shdr.sh_offset, shdr.sh_size);
        lseek(fd_, shdr.sh_offset, SEEK_SET);
        *buffer = malloc(shdr.sh_size);
        *buffer_size = read(fd_, *buffer, shdr.sh_size);
    }

private:
    bool ReadElfHeader() {
        ssize_t rc = read(fd_, &header_, sizeof(ElfW_Ehdr));
        if (rc < 0) {
            LOGE("Can not read the elf header.");
            return false;
        }
    #if DEBUG
        DumpElfHeader(header_);
    #endif 
        return true;
    }

    bool VerifyElfHeader() {
        if (header_.e_ident[EI_MAG0] != ELFMAG0 || header_.e_ident[EI_MAG1] != ELFMAG1
                || header_.e_ident[EI_MAG2] != ELFMAG2 || header_.e_ident[EI_MAG3] != ELFMAG3) {
            LOGE("It's not a valid elf file.");
            return false;
        }
        return true;
    }

    bool ReadProgramHeaders() {
        phdr_.clear();
        phdr_.reserve(header_.e_phnum);

        lseek(fd_, header_.e_phoff, SEEK_SET);
        size_t rc = -1;
        for (int i = 0; i < header_.e_phnum; i++) {
            ElfW_Phdr phdr;
            rc = read(fd_, &phdr, sizeof(ElfW_Phdr));
    #if DEBUG
            DumpPhdr(phdr);
    #endif
            if (rc < 0) {
                LOGE("can not read Phdr(#%d).", i);
                return false; // break
            }
            phdr_.push_back(phdr);
        }

        return true; 
    }

    bool ReadSectionHeaders() {
        shdr_.clear();
        shdr_.reserve(header_.e_shnum);

        // read section headers
        lseek(fd_, header_.e_shoff, SEEK_SET);
        size_t rc = -1;
        for (int i = 0; i < header_.e_shnum; i++) {
            ElfW_Shdr shdr;
            rc = read(fd_, &shdr, sizeof(ElfW_Shdr));
            if (rc < 0) {
                LOGE("can not read Shdr(#%d).", i);
                return false; // break
            }
            shdr_.push_back(shdr);
        }

        // read section header strings table
        ElfW_Shdr shdr = shdr_[header_.e_shstrndx];
        shstrtab_size_ = shdr.sh_size;
        shstrtab_ = static_cast<unsigned char*>(malloc(shstrtab_size_));
        DumpShdr(shdr);
        lseek(fd_, shdr.sh_offset, SEEK_SET);
        rc = read(fd_, shstrtab_, shstrtab_size_);
        if (rc < 0) {
            LOGE("can not read section header string table.");
            return false; 
        }

    #if DEBUG
        for (auto &shdr : shdr_) {
            DumpShdr(shdr, true /* resolve_name */);
        }
    #endif

        return true; 
    }

    bool ReadDynamicSection() {
        ElfW_Shdr* dynamic_shdr = FindShdrByType(SHT_DYNAMIC);
        if (dynamic_shdr == nullptr) {
            LOGE("can not find .dynamic in section header tables.");
            return false;
        }

        ElfW_Phdr* dynamic_phdr = FindPhdrByType(PT_DYNAMIC);
        if (dynamic_phdr == nullptr) {
            LOGE("can not find PT_DYNAMIC int program header tables.");
            return false;
        }

        if (dynamic_shdr->sh_offset != dynamic_phdr->p_offset) {
            LOGE(".dynmaic section has invalid offset: 0x%08lx, expected to match PT_DYNAMIC offset: 0x%08lx.", 
                    dynamic_shdr->sh_offset, dynamic_phdr->p_offset);
            return false;
        }
        if (dynamic_shdr->sh_size != dynamic_phdr->p_filesz) {
            LOGE(".dynmaic section has invalid size: 0x%08lx, expected to match PT_DYNAMIC offset: 0x%08lx.", 
                    dynamic_shdr->sh_size, dynamic_phdr->p_filesz);
            return false;
        }

        if (dynamic_shdr->sh_link >= shdr_.size()) {
            LOGE(".dynmaic section has invalid sh_link: %u.", dynamic_shdr->sh_link);
            return false;
        }

        // read .dynstr section
        ElfW_Shdr dymstr_shdr = shdr_[dynamic_shdr->sh_link];
        dynstr_size_ = dymstr_shdr.sh_size;
        dynstr_ = static_cast<unsigned char*>(malloc(dynstr_size_));
        lseek(fd_, dymstr_shdr.sh_offset, SEEK_SET);
        size_t rc = read(fd_, dynstr_, dynstr_size_);
        if (rc < 0) {
            LOGE("can not read .dynstr section, shdr index=%d.", dynamic_shdr->sh_link);
            return false; 
        }
    #if DEBUG
        LOGD(".dynstr(offset=0x%08lx, size=%ld):", dymstr_shdr.sh_offset, dymstr_shdr.sh_size);
        hexdump((void*)dynstr_, dynstr_size_);
    #endif

        // read .dynamic section
        size_t dynamic_size = dynamic_shdr->sh_size / sizeof(ElfW_Dyn);
        dynamic_.clear();
        dynamic_.reserve(dynamic_size);

        lseek(fd_, dynamic_shdr->sh_offset, SEEK_SET);
        rc = -1;
        for (int i = 0; i < dynamic_size; i++) {
            ElfW_Dyn dyn;
            rc = read(fd_, &dyn, sizeof(ElfW_Dyn));
            if (rc < 0) {
                LOGE("can not read Dyn(#%d).", i);
                return false; // break
            }
    #if DEBUG
            DumpDynamic(dyn, true /* resolve_name */);
    #endif
            dynamic_.push_back(dyn);
        }

        return true;
    }

    bool ReadSymbols() {
        // read .strtab section
        ElfW_Shdr* strtab_shdr = FindShdrByName(".strtab");
        if (strtab_shdr == nullptr) {
            LOGE("can not find .strtab in section header tables.");
            return false;
        }
        strtab_size_ = strtab_shdr->sh_size;
        strtab_ = static_cast<unsigned char*>(malloc(strtab_size_));
        lseek(fd_, strtab_shdr->sh_offset, SEEK_SET);
        size_t rc = read(fd_, strtab_, strtab_size_);
        if (rc < 0) {
            LOGE("can not read .strtab section.");
            return false; 
        }
    #if DEBUG
        LOGD(".strtab(offset=0x%08lx, size=%ld):", strtab_shdr->sh_offset, strtab_shdr->sh_size);
        hexdump((void*)strtab_, strtab_size_);
    #endif

        // read .symtab section
        ElfW_Shdr* symtab_shdr = FindShdrByType(SHT_SYMTAB);
        if (symtab_shdr == nullptr) {
            LOGE("can not find .dynamic in section header tables.");
            return false;
        }
        size_t symtab_size = symtab_shdr->sh_size / sizeof(ElfW_Sym);
        symtab_.clear();
        symtab_.reserve(symtab_size);

        lseek(fd_, symtab_shdr->sh_offset, SEEK_SET);
        for (int i = 0; i < symtab_size; i++) {
            ElfW_Sym sym;
            rc = read(fd_, &sym, sizeof(ElfW_Sym));
            if (rc < 0) {
                LOGE("can not read Sym(#%d).", i);
                return false; // break
            }
    #if DEBUG
            DumpSymbol(sym, true /* resolve_name */);
    #endif
            symtab_.push_back(sym);
        }

        return true; 
    }

    // for debug
    void DumpElfHeader(const ElfW_Ehdr& header) {
        LOGD("ELF Header:");
        LOGD("\te_type(Type of file (see ET_*)): %d", header.e_type);
        LOGD("\te_machine(Required architecture for this file (see EM_*)): %d", header.e_machine);
        LOGD("\te_version(Must be equal to 1): %d", header.e_version);
        LOGD("\te_entry(Address to jump to in order to start program): 0x%08lx", header.e_entry);
        LOGD("\te_phoff(Progrom header table file offset): 0x%08lx", header.e_phoff);
        LOGD("\te_shoff(Section header table file offset): 0x%08lx", header.e_shoff);
        LOGD("\te_flags: %d", header.e_flags);
        LOGD("\te_ehsize(ELF Header size in bytes): %d", header.e_ehsize);
        LOGD("\te_phentsize(Program header table entry size): %d", header.e_phentsize);
        LOGD("\te_phnum(Program header table entry count): %d", header.e_phnum);
        LOGD("\te_shentsize(Section header entry size): %d", header.e_shentsize);
        LOGD("\te_shnum(Section header entry count): %d", header.e_shnum);
        LOGD("\te_shstrndx(Section header string table index): %d", header.e_shstrndx);
    }

    void DumpPhdr(const ElfW_Phdr& phdr) {
        /*
        p_type: Type of segment
        p_offset: FileOffset where segment is located, in bytes
        p_vaddr: Virtual Address of beginning of segment
        p_paddr: Physical address of beginning of segment (OS-specific)
        p_filesz: Num. of bytes in file image of segment (may be zero)
        p_memsz: Num. of bytes in mem image of segment (may be zero)
        p_flags: Segment flags
        p_align: Segment alignment constraint
        */
        char type[32];
        if (phdr.p_type == PT_LOAD) {
            strcpy(&type[0], "PT_LOAD");
        } else {
            sprintf(&type[0], "0x%x", phdr.p_type);
        }
        LOGD("[phdr] p_offset=0x%08lx, p_vaddr=0x%08lx, p_paddr=0x%08lx, p_filesz=0x%08lx, p_memsz=0x%08lx, p_flags=%d, p_align=0x%lx, p_type=%s", 
                    phdr.p_offset, phdr.p_vaddr, phdr.p_paddr, phdr.p_filesz, phdr.p_memsz, phdr.p_flags, phdr.p_align, type);
    }

    void DumpShdr(const ElfW_Shdr& shdr, bool resolve_name = false) {
        /*
        sh_name: Section name
        sh_type: Section type (SHT_*)
        sh_flags: Section flags (SHF_*)
        sh_addr: Address where section is to be loaded
        sh_offset: File offset of section data, in bytes
        sh_size: Size of section, in bytes
        sh_link: Section type-specific header table index link
        sh_info: Section type-specific extra information
        sh_addralign: Section address alignment
        sh_entsize: Size of records contained within the section
        */
        char sh_name[256];
        if (resolve_name && shstrtab_ != nullptr) {
            const unsigned char* name = shstrtab_ + shdr.sh_name;
            strcpy(&sh_name[0], (const char*)name);
        } else {
            sprintf(&sh_name[0], "%d", shdr.sh_name);
        }
        LOGD("[shdr] sh_type=%d, sh_flags=%lu, sh_addr=0x%08lx, sh_offset=0x%08lx, sh_size=0x%08lx, sh_link=%d, sh_info=%d, sh_addralign=%lu, sh_entsize=0x%lx, sh_name=%s",
                    shdr.sh_type,  shdr.sh_flags, shdr.sh_addr, shdr.sh_offset, shdr.sh_size, shdr.sh_link, shdr.sh_info, shdr.sh_addralign, shdr.sh_entsize, sh_name);
    }

    void DumpDynamic(const ElfW_Dyn& dyn, bool resolve_name = false) {
        /*
        d_tag: Dynamic entry type
        d_un: Integer or Address value
        */
        bool need_resolve_name = resolve_name && dynstr_ != nullptr;
        switch (dyn.d_tag) {
            case DT_SONAME:
                if (need_resolve_name) LOGD("[Dynamic] d_tag=%ld, DT_SONAME: %s", dyn.d_tag, dynstr_ + dyn.d_un.d_ptr);
                break;
            case DT_RUNPATH:
                if (need_resolve_name) LOGD("[Dynamic] d_tag=%ld, DT_RUNPATH: %s", dyn.d_tag, dynstr_ + dyn.d_un.d_ptr);
                break;
            case DT_NEEDED:
                if (need_resolve_name) LOGD("[Dynamic] d_tag=%ld, DT_NEEDED: %s", dyn.d_tag, dynstr_ + dyn.d_un.d_ptr);
                break;
            case DT_SYMTAB:
                LOGD("[Dynamic] d_tag=%ld, DT_SYMTAB: va=0x%08lx", dyn.d_tag, dyn.d_un.d_ptr);
                break;
            default:
                LOGD("[Dynamic] d_tag=%ld, d_un=0x%08lx(0x%08lx)", dyn.d_tag, dyn.d_un.d_val, dyn.d_un.d_ptr);
        }
    }

    void DumpSymbol(const ElfW_Sym& sym, bool resolve_name = false) {
        /*
        st_name: string tbl index
        st_info: Symbol type and binding
        st_other: Symbol visibility
        st_shndx: Section index
        st_value: Symbol value
        st_size: Symbol size
        */
        char st_name[256];
        if (resolve_name && strtab_ != nullptr && sym.st_name > 0) {
            const unsigned char* name = strtab_ + sym.st_name;
            strcpy(&st_name[0], (const char*)name);
        } else {
            sprintf(&st_name[0], "%d", sym.st_name);
        }
        LOGD("[Symbol] st_info.type=%d, st_info.binding=%d, st_other=%d, st_shndx=%d, st_value=0x%08lx, st_size=0x%08lx, st_name=%s",
            header_.e_ident[EI_CLASS] == ELFCLASS32 ? ELF32_ST_TYPE(sym.st_info) : ELF64_ST_TYPE(sym.st_info),
            header_.e_ident[EI_CLASS] == ELFCLASS32 ? ELF32_ST_BIND(sym.st_info) : ELF64_ST_BIND(sym.st_info),
            sym.st_other, sym.st_shndx, sym.st_value, sym.st_size, st_name);
    }

public: // TODO: only public to ElfWriter
    int fd_;
    std::string name_;
    ElfW_Ehdr header_;

    std::vector<ElfW_Phdr> phdr_; // the program header table
    std::vector<ElfW_Shdr> shdr_; // the section header table

    std::vector<ElfW_Dyn> dynamic_; // .dynamic
    std::vector<ElfW_Sym> symtab_; // .symtab

    unsigned char* shstrtab_; // .shstrtab
    size_t shstrtab_size_; // size of .shstrtab

    unsigned char* dynstr_; // .dynstr
    size_t dynstr_size_; // size of .dynstr

    unsigned char* strtab_; // .strtab
    size_t strtab_size_; // size of .strtab

    size_t file_size_; // size of elf file

}; // class ElfReader

class ElfReader32 : public ElfReader<Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr, Elf32_Dyn, Elf32_Sym> {
public:
    ElfReader32() : ElfReader<Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr, Elf32_Dyn, Elf32_Sym>() {}
};

class ElfReader64 : public ElfReader<Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr, Elf64_Dyn, Elf64_Sym> {
public:
    ElfReader64() : ElfReader<Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr, Elf64_Dyn, Elf64_Sym>() {}
};

}


#endif