#ifndef CDL_MemoryMapper_hpp
#define CDL_MemoryMapper_hpp

#include <stdio.h>
#include "codeDataLogger.hpp"
namespace codeDataLogger {
    //
    // # CodeDataLogger Mapping (which memory address is being referenced)
    //
    struct CDMapResult
    {
        eCDLog_AddrType type;
        unsigned addr;
    };

    //
    // # CDL_Platform
    //
    struct CDL_Platform
    {
        char* platformName;
        unsigned memory;
    };
    
    CDMapResult CDMap(const unsigned P, CDL_Platform* cart);
    
}

#endif /* CDL_MemoryMapper_hpp */
