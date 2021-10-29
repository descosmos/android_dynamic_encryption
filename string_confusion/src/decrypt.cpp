#include <unistd.h>
#include <fcntl.h>
#include <elf.h>

extern "C"{
    /**
     * Function decrypyt is used to decrypt the encrypted .so file.
    */
    __attribute__((constructor)) void decrypt() {
        // TODO
    }
}