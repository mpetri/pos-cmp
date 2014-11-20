#pragma once

#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace utils
{

bool
directory_exists(std::string dir)
{
    struct stat sb;
    const char* pathname = dir.c_str();
    if (stat(pathname, &sb) == 0 && (S_IFDIR&sb.st_mode)) {
        return true;
    }
    return false;
}

bool
file_exists(std::string file_name)
{
    std::ifstream in(file_name);
    if (in) {
        in.close();
        return true;
    }
    return false;
}

bool
symlink_exists(std::string file)
{
    struct stat sb;
    const char* filename = file.c_str();
    if (stat(filename, &sb) == 0 && (S_IFLNK&sb.st_mode)) {
        return true;
    }
    return false;
}

void
create_directory(std::string dir)
{
    if (!directory_exists(dir)) {
        if (mkdir(dir.c_str(),0777) == -1) {
            perror("could not create directory");
            exit(EXIT_FAILURE);
        }
    }
}


struct vbyte_coder {
    static size_t encode_num(uint32_t num,uint8_t* out)
    {
        size_t written_bytes = 0;
        uint8_t w = num & 0x7F;
        num >>= 7;
        while (num > 0) {
            w |= 0x80; // mark overflow bit
            *out = w;
            ++out;
            w = num & 0x7F;
            num >>= 7;
            written_bytes++;
        }
        *out = w;
        ++out;
        written_bytes++;
        return written_bytes;
    }
    static uint32_t decode_num(const uint8_t*& in)
    {
        uint32_t num = 0;
        uint8_t w=0;
        uint32_t shift=0;
        do {
            w = *in;
            in++;
            num |= (((uint32_t)(w&0x7F))<<shift);
            shift += 7;
        } while ((w&0x80) > 0);
        return num;
    }
    static void encode(const uint32_t* A,size_t n,uint32_t* out,size_t& written_u32s)
    {
        uint8_t* out_bytes = (uint8_t*) out;
        size_t written_bytes = 0;
        for (size_t i=0; i<n; i++) {
            size_t written = encode_num(A[i],out_bytes);
            out_bytes += written;
            written_bytes += written;
        }
        written_u32s = written_bytes/4;
        if (written_bytes%4 != 0) written_u32s++;
    }
    static void decode(const uint32_t* in,size_t n,uint32_t* out)
    {
        const uint8_t* in_bytes = (const uint8_t*) in;
        for (size_t i=0; i<n; i++) {
            *out = decode_num(in_bytes);
            out++;
        }
    }
};


} // end of util namespace
