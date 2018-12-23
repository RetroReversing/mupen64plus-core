#ifndef codeDataLogger_hpp
#define codeDataLogger_hpp

#include <stdio.h>
#include <vector>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

namespace codeDataLogger {
    bool writeExample();
    bool writeCDL( const std::string &filename, const std::string cdlpath);
    bool loadCDL( const std::string &filename, const std::string &cdlpath);
    void differenceBetweenCurrentAndPreviousMemory(std::ostream* file, bool dumpWholeROM, int memory_size, unsigned char* current_memory, unsigned char* old_memory, bool (*shouldWriteBlock)(int,int,int,int,int), unsigned char* non_mask_memory);

    void calculateCDLStatistics(const std::string &filename, bool writeToFile, bool wholeROM);

    enum eCDLog_AddrType
    {
        eCDLog_AddrType_ROM, eCDLog_AddrType_HRAM, eCDLog_AddrType_WRAM, eCDLog_AddrType_CartRAM,
        eCDLog_AddrType_None, eCDLog_AddrType_TileRAM, eCDLog_AddrType_BGMap,eCDLog_AddrType_OAM,
        eCDLog_AddrType_Sound
    };
    
    enum eCodeLog_Flags
    {
        eCodeLog_Flags_Jump_True = 1,
        eCodeLog_Flags_Jump_False = 2
    };
    
//    Might also be useful to count the cycles for each byte of code

    enum eCDLog_Flags
    {
        eCDLog_Flags_ExecFirst = 1,
        eCDLog_Flags_ExecOperand = 2,
        eCDLog_Flags_Data = 4,
        eCDLog_Flags_BlockEnd = 8,
        //eCDLog_Flags_BlockStart = 8,
        //eCDLog_Flags_BlockEnd = 16,
        eCDLog_Flags_Graphics = 16,
        eCDLog_Flags_BGMap = 32,
        eCDLog_Flags_OAM = 64,
        eCDLog_Flags_Sound = 128,
        eCDLog_Flags_None = 99
    };
    typedef void (*CDCallback)(int32_t addr, eCDLog_AddrType addrtype, eCDLog_Flags flags);
}

#endif /* codeDataLogger_hpp */
