#include "CDL.hpp"

unsigned int hex_to_int(string str) {
    unsigned int x;   
    std::stringstream ss;
    ss << std::hex << str;
    ss >> x;
    return x;
}

void printBytes(uint8_t* mem, uint32_t cartAddr) {
    for (int i=0; i<=0x18; i++) {
        printf(" %d:%#02x ", i, mem[cartAddr+i]);
        if ((i+1)%4 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}