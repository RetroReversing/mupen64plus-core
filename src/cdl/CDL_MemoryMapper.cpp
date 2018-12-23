// #include "gambatte.h"
// #include "mem/cartridge.h"
#include "CDL_MemoryMapper.hpp"

void CDL_PrintMemoryLoc(uint32_t address) {//, void* mem_base) {
    unsigned char* bytes = ((uint8_t*) address);
    // printf("\nBytes: %x %x %x %x ",bytes[3], bytes[2], bytes[1], bytes[0]);
    // printf("\nBytes: %x %x %x %x ",mem[3], mem[2], mem[1], mem[0]);
    // printf("\nAddress: %d ",268435456-address);
    // mem_base = MEM_BASE_PTR(mem_base);
    // if (address < RDRAM_MAX_SIZE) {
    //     printf("RDRAM: %d \n", address);
    //         //mem = (uint32_t*)((uint8_t*)mem_base + (address - MM_RDRAM_DRAM + MB_RDRAM_DRAM));
    //     }
    // else if (address >= MM_CART_ROM) {
    //     printf("CART ROM %d \n", address);
    // }
    // else if ((address & UINT32_C(0xfe000000)) ==  MM_DD_ROM) {
    //     printf("DD ROM %d \n", address);
    //     }
    // else if ((address & UINT32_C(0xffffe000)) == MM_RSP_MEM) {   
    //     printf("Reality Signal Processor MEM %d \n", address);
    // }
    // else {
    //     printf("UNKNOWN: %d \n", address);
    // }
}

namespace codeDataLogger {
    //  P is address of memory we want to access
    // cart is CDL_Platform to access memory
    CDMapResult CDMap(const unsigned P, CDL_Platform* cart)
    {
        printf("TODO: map P");
        CDMapResult ret = { eCDLog_AddrType_None };
        return ret;
//         if(P<0x4000)
//         {
//             //
//             //  First ROM Bank (fixed)
//             //
//             CDMapResult ret = { eCDLog_AddrType_ROM, P };
//             return ret;
//         }
//         else if(P<0x8000)
//         {
//             //
//             //  Switchable ROM bank 1+
//             //
//             unsigned bank = cart->rmem(P>>12) - cart->rmem(0);
//             unsigned addr = P+bank;
//             CDMapResult ret = { eCDLog_AddrType_ROM, addr };
//             return ret;
//         }
//         else if (P<0x9800) {
//             //
//             // Character RAM (Tiles)
//             //
//             CDMapResult ret = {eCDLog_AddrType_TileRAM, P};
//             return ret;
//         }
//         else if (P< 0x9C00) {
//             //
//             // BG Map Data 1
//             //
//             CDMapResult ret = {eCDLog_AddrType_BGMap, P};
//             return ret;
            
//         }
//         else if(P<0xA000) {
//             //
//             // BG Map Data 2
//             CDMapResult ret = {eCDLog_AddrType_BGMap, P};
//             return ret;
//         }
//         else if(P<0xC000)
//         {
//             //  Extendible RAM on cartridge
//             if(cart->wsrambankptr())
//             {
//                 //not bankable. but. we're not sure how much might be here
//                 unsigned char *data;
//                 int length;
//                 bool has = cart->getMemoryArea(3,&data,&length);
//                 unsigned addr = P&(length-1);
//                 if(has && length!=0)
//                 {
//                     CDMapResult ret = { eCDLog_AddrType_CartRAM, addr };
//                     return ret;
//                 }
//             }
//         }
//         else if(P<0xE000)
//         {
//             //  Working RAM (multiple banks)
//             unsigned bank = cart->wramdata(P >> 12 & 1) - cart->wramdata(0);
//             unsigned addr = (P&0xFFF)+bank;
//             CDMapResult ret = { eCDLog_AddrType_WRAM, addr };
//             return ret;
//         }
//         else if (P>=0xFE00 && P<=0xFE9F) {
//             CDMapResult ret = {eCDLog_AddrType_OAM, P};
//             return ret;
//         }
//         else if (P>=0xFF10 && P<=0xFF26) {
//             CDMapResult ret = {eCDLog_AddrType_Sound, P};
//             return ret;
//         }
        
//         else if (P>=0xFF26 && P<=0xFF3F) {
//             CDMapResult ret = {eCDLog_AddrType_Sound, P};
//             return ret;
//         }
//         else if(P<0xFF80) {}
//         else
//         {
//             ////this is just for debugging, really, it's pretty useless
// //            CDMapResult ret = { eCDLog_AddrType_HRAM, (P-0xFF80) };
// //            return ret;
//         }
        
//         CDMapResult ret = { eCDLog_AddrType_None };
//         return ret;
    }
}
