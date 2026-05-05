#include "Elf_Loader.h"
#include <cstring>
#include <cstdio>
#include <sys/mman.h>
#include <unistd.h>
#include <elf.h>
#include <fcntl.h>

namespace com {
namespace arenax3 {

ElfLoader::ElfLoader() : loaded_base(nullptr), entry_point(0), mem_size(0) {}

ElfLoader::~ElfLoader() {
    unload();
}

bool ElfLoader::loadFromFile(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) return false;
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    uint8_t* file_data = new uint8_t[file_size];
    fread(file_data, 1, file_size, fp);
    fclose(fp);
    
    bool result = loadFromMemory(file_data, static_cast<size_t>(file_size));
    delete[] file_data;
    
    return result;
}

bool ElfLoader::loadFromMemory(const uint8_t* data, size_t size) {
    if (size < sizeof(Elf64_Ehdr)) return false;
    
    const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(data);
    
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        return false;
    }
    
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) return false;
    if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) return false;
    
    mem_size = 0;
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        const Elf64_Phdr* phdr = reinterpret_cast<const Elf64_Phdr*>(
            data + ehdr->e_phoff + i * ehdr->e_phentsize);
        if (phdr->p_type == PT_LOAD) {
            uint64_t segment_end = phdr->p_vaddr + phdr->p_memsz;
            if (segment_end > mem_size) mem_size = segment_end;
        }
    }
    
    loaded_base = mmap(nullptr, mem_size, PROT_NONE, 
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (loaded_base == MAP_FAILED) return false;
    
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        const Elf64_Phdr* phdr = reinterpret_cast<const Elf64_Phdr*>(
            data + ehdr->e_phoff + i * ehdr->e_phentsize);
        if (phdr->p_type == PT_LOAD) {
            uint64_t segment_addr = reinterpret_cast<uint64_t>(loaded_base) + phdr->p_vaddr;
            
            mprotect(reinterpret_cast<void*>(segment_addr), phdr->p_memsz,
                    PROT_READ | PROT_WRITE);
            
            memcpy(reinterpret_cast<void*>(segment_addr),
                   data + phdr->p_offset, phdr->p_filesz);
            
            if (phdr->p_memsz > phdr->p_filesz) {
                memset(reinterpret_cast<void*>(segment_addr + phdr->p_filesz), 0,
                       phdr->p_memsz - phdr->p_filesz);
            }
            
            mprotect(reinterpret_cast<void*>(segment_addr), phdr->p_memsz,
                    (phdr->p_flags & PF_R ? PROT_READ : 0) |
                    (phdr->p_flags & PF_W ? PROT_WRITE : 0) |
                    (phdr->p_flags & PF_X ? PROT_EXEC : 0));
        }
    }
    
    entry_point = reinterpret_cast<void*>(
        reinterpret_cast<uint64_t>(loaded_base) + ehdr->e_entry);
    
    return true;
}

void* ElfLoader::getEntryPoint() const {
    return entry_point;
}

void* ElfLoader::getSymbol(const char* name) const {
    if (!loaded_base) return nullptr;
    
    return nullptr;
}

void ElfLoader::unload() {
    if (loaded_base) {
        munmap(loaded_base, mem_size);
        loaded_base = nullptr;
        entry_point = nullptr;
        mem_size = 0;
    }
}

bool ElfLoader::isLoaded() const {
    return loaded_base != nullptr;
}

int ElfLoader::execute() {
    if (!entry_point) return -1;
    
    typedef int (*entry_func_t)();
    entry_func_t main_func = reinterpret_cast<entry_func_t>(entry_point);
    return main_func();
}

} // namespace arenax3
} // namespace com