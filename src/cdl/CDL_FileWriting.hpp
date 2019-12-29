#ifndef CDL_FileWriting_hpp
#define CDL_FileWriting_hpp

#include <stdio.h>
#include <vector>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

namespace codeDataLogger {
    void copyOverOtherStatesDiffContents(std::ostream* file);
    void writeInteger(std::ofstream* file, int num);
    void writeBlock(std::ofstream* file, const char *cdl_block, int size);
    uint8_t readInteger8(std::ifstream* file);
    uint32_t readInteger32(std::ifstream* file);
    std::string readString(std::ifstream* file, int num);
    void readBlock(std::ifstream* file, char* buffer);
    void writeDataBytes(std::ostream* file, int start_range_address, int range_size, unsigned char* memory);
    void writeDataBytes(const std::ofstream& file, int start_range_address, int range_size, unsigned char* memory);
    
}
#endif /* CDL_FileWriting_hpp */
