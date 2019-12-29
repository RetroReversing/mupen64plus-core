#include <vector>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <iostream>
#include "codeDataLogger.hpp"
#include "CDL_FileWriting.hpp"

#define VRAM_SIZE 0x1fff
#define TILE_MAP_SIZE 6144 // 6KB of Tile data (pixel data)
#define Tile_MAP_START 0x8000
namespace codeDataLogger {
    int cdl_romsize;
    int cdl_wramsize;
    int cdl_cartramsize;
    float cdl_mapped_pc = 0.0;



    extern unsigned char *cdl_memchunk_;
    extern unsigned char *cdl_memchunk_cartram;
    extern unsigned char *cdl_memchunk_workram;
    extern unsigned char *full_romchunk_;
    extern unsigned char *full_vram_;
    extern unsigned char *full_tile_map_;

    unsigned char *cdl_memchunk_rom_previous = nullptr;
    unsigned char *cdl_memchunk_vram_previous = nullptr;
    unsigned char *cdl_memchunk_tile_previous = nullptr;


    const int NUM_OF_CDL_BLOCKS = 3;

    bool writeCDL( const std::string &filename, const std::string cdlpath);

    //std::ifstream sourcecdlDiff;
    bool has_loaded_state = false;
    std::string last_loaded_cdl = "";

    void resetDiff() {
        memcpy(cdl_memchunk_rom_previous,codeDataLogger::cdl_memchunk_, codeDataLogger::cdl_romsize);
        //memcpy(cdl_memchunk_vram_previous,full_vram_, VRAM_SIZE);
        memcpy(cdl_memchunk_tile_previous,codeDataLogger::full_tile_map_, TILE_MAP_SIZE);
    }
    typedef std::bitset<sizeof(long int)> IntBits;

    bool is_data(int value) {
        bool is_set = IntBits(value).test(2);
        return is_set;
    }

    bool is_code(int value) {
        bool is_set = IntBits(value).test(0) || IntBits(value).test(1);
        return is_set;
    }

//    bool is_first_byte_of_block(int value) {
//        return IntBits(value).test(3);
//    }

    bool is_last_byte_of_block(int value) {
        return IntBits(value).test(3);
    }

    bool is_graphics(int value) {
        return IntBits(value).test(4);
    }

    bool is_bgmap(int value) {
        return IntBits(value).test(5);
    }

    bool is_oam(int value) {
        return IntBits(value).test(6);
    }

    bool is_sound(int value) {
        return IntBits(value).test(7);
    }

    //
    // # Write a range of bytes in json format (startaddress -> end_address) = (data or code)
    //    Useful for tracking down new functions that have been executed since the last save
    //    and also new data accessed since last save (sprites, sounds, text etc)
    //
    void writeRange(std::ostream* file, int start_range_address, int previous_address, int type_of_range_data, unsigned char* memory_chunk) {
        *file << "\"";
        *file << std::hex;
        if (start_range_address == previous_address)
        {
            *file << "0x" << start_range_address;
        }
        else
        {
            *file << "0x" << start_range_address << "-" << "0x" << previous_address;
        }
        *file << std::dec;
        *file << "\": ";

        int range_size = (previous_address-start_range_address) +1;
        std::string type_of_data_range = "unknown";
        if (is_code(type_of_range_data)) {
            type_of_data_range = "code";
        }
        else if (is_graphics(type_of_range_data)) {
            type_of_data_range = "graphics";
        } else if (is_bgmap(type_of_range_data)) {
            type_of_data_range = "bgmap";
        } else if (is_oam(type_of_range_data)) {
            type_of_data_range = "oam";
        } else if (is_sound(type_of_range_data)) {
            type_of_data_range = "sound";
        }
        
        else if (is_data(type_of_range_data)) {
            type_of_data_range = "data";
        }
        

        *file << " {\"len\":" << range_size << ", \"type\": \"" << type_of_data_range << "\"";
        *file << ", \"data\": [";
        writeDataBytes(file, start_range_address, range_size, memory_chunk);
        *file << "]";
        *file << "}";

        *file << ",\n";

    }



    void writePreviousRange(std::ostream* file, int start_range_address, int previous_address, int type_of_range_data, unsigned char* memory_chunk) {
        if (previous_address !=-1)
        {
            writeRange(file, start_range_address, previous_address, type_of_range_data, memory_chunk);
            writeRange(&std::cout, start_range_address, previous_address, type_of_range_data, memory_chunk);
        }
    }

    void genericDiff(std::ostream* file, int memory_size, unsigned char* new_memory, unsigned char* old_memory) {
        *file << std::hex;
        int previous_address = -1;
        int start_range_address = -1;
        for (int i=0; i< memory_size; i++) {
            int newValue = new_memory[i];
            int oldValue = old_memory[i];
            if (newValue == oldValue) continue;


            if (previous_address == (i-1) ) {
                // We are 1 byte up from the previous address and we are the same type (code or data) so we are part of a range
            }
            else {
                // first write the previous range
                writePreviousRange(file, start_range_address, previous_address, 0x04, new_memory);

                //   New range
                start_range_address = i;
//                *file << newValue;
            }
            previous_address = i;
        }
        *file << std::dec;

    }

#include <string>
#include <iostream>
#include <sstream>
    void isStringInFile(std::ifstream* file, std::string &searchLine) {
        //char line[256];

        if(file){
            std::string line;
            while (std::getline(*file, line)) {
                std::cout << line << std::endl;
            }
        }
    }
//
// Write VRAM out to the console
//

    void writeVRam() {
        // std::cout << "\nVRAM:\n";
        // genericDiff(&std::cout, VRAM_SIZE, codeDataLogger::full_vram_, cdl_memchunk_vram_previous);
        //writeDataBytes(&std::cout, 0, VRAM_SIZE, codeDataLogger::full_vram_);
    }

    bool shouldWriteOutThePreviousBlockForTileMap(int newValue, int oldValue, int current_address, int previous_address) {
        return false;
    }

    void writeTileMap(const std::string &filename) {
//         std::cout << "\nTile Data:\n";
//         std::ofstream file((filename+".tilemap").c_str(), std::ios_base::binary);
//         // std::ofstream& file = std::ofstream((filename+".tilemap").c_str(), std::ios_base::binary);
//         //genericDiff(&std::cout, TILE_MAP_SIZE, codeDataLogger::full_tile_map_, cdl_memchunk_tile_previous);
//         int count=1;
//         for (int i=0; i<TILE_MAP_SIZE; i+=16)
//         {
//             writeDataBytes(file, i, 16, codeDataLogger::full_vram_);
// //            std::cout << ",\n"<<count<<":";
//             count++;
//         }
//         // now we should write out to a file in 16 byte chunks so we can diff on a tile level!
//         const std::ofstream& difffile = std::ofstream((filename+".tilemap.json").c_str(), std::ios_base::binary);;
        //differenceBetweenCurrentAndPreviousMemory(&std::cout, false, TILE_MAP_SIZE, codeDataLogger::full_tile_map_, cdl_memchunk_tile_previous, &shouldWriteOutThePreviousBlockForTileMap);
    }

    //
    // Writes a json diff
//    non_mask_memory is the memory of the actual rom/ram not just a bitmask ontop of it
    //
    void differenceBetweenCurrentAndPreviousMemory(std::ostream* file, bool dumpWholeROM, int memory_size, unsigned char* current_memory, unsigned char* old_memory, bool (*shouldWriteBlock)(int,int,int,int,int), unsigned char* non_mask_memory) {
        int previous_address = -1;
        int start_range_address = -1;
        int previous_range_data_type=0;
        for (int i=0; i< memory_size; i++) {
            int newValue = current_memory[i];
            int oldValue = old_memory[i];

            //    Check if we are the same as the old value, we only want differences
            if (newValue == oldValue && !dumpWholeROM) continue;

            if (shouldWriteBlock(newValue, oldValue,i,previous_address, previous_range_data_type)) {
                // first write the previous range
                writePreviousRange(file, start_range_address, previous_address, previous_range_data_type, non_mask_memory);
                if (!dumpWholeROM) {
                    // if we are in a diff then we can safely assume that the previous address is the end of a unique block of data
                    current_memory[previous_address] |= eCDLog_Flags_BlockEnd;
                }

                //   New range
                start_range_address = i;
            }

            previous_address = i;
            previous_range_data_type = newValue;
        }
        // write last range
        writePreviousRange(file, start_range_address, previous_address, previous_range_data_type, codeDataLogger::full_romchunk_);

    }

//
// TODO Next step is to make sure all bytes in a block are definetly the same type!
//

    bool shouldWriteOutThePreviousBlockForROM(int newValue, int previousValue, int current_address, int previous_address, int previous_address_value) {
        bool new_value_is_data = is_data(newValue);
        bool new_value_is_last_of_block = is_last_byte_of_block(newValue);

        if (new_value_is_last_of_block) return true;

        if (previous_address != (current_address-1)) {
//
// We have found a block that is new since the previous address is not just 1 byte previous
//  This means the memory was changed independently of the previous bytes
//  So we now mark the previous byte as being the end of a block
//
            return true;
        }

        if (is_graphics(newValue) != is_graphics(previous_address_value)) {
            // one block was graphics and the other wasn't so we do want to split them out
            return true;
        }

        if (is_bgmap(newValue) != is_bgmap(previous_address_value)) {
            // one block was bgmap and the other wasn't so we do want to split them out
            return true;
        }

        if (is_oam(newValue) != is_oam(previous_address_value)) {
            // one block was oam and the other wasn't so we do want to split them out
            return true;
        }

        if (is_sound(newValue) != is_sound(previous_address_value)) {
            // one block was oam and the other wasn't so we do want to split them out
            return true;
        }

        if (new_value_is_data != is_data(previous_address_value)) {
            return true;
        }

        if (is_code(newValue) != is_code(previous_address_value)) {
            // one block was bgmap and the other wasn't so we do want to split them out
            return true;
        }
        return false;
    }


    // idea to get first byte read that isn't executable (start of block)
    //    and last address read before executing code (end of block)

    void writeDiff(const std::string &filename, float mapped_pc, long bytesMapped, bool dumpWholeROM) {
        //        if (!has_loaded_state) return;
        if (cdl_mapped_pc < mapped_pc || dumpWholeROM) {
            printf("%s",filename.c_str());
            printf("\nIncreased Mapped Percentage to %f from %f\n", mapped_pc, cdl_mapped_pc);

            std::ofstream file;
            std::string diff_filename = filename;
            if (dumpWholeROM)
            {
                diff_filename+=".gb.json";
            } else
            {
                diff_filename+=".cdldiff.json";
            }

            if (last_loaded_cdl == diff_filename) {
                file = std::ofstream((filename+".cdldiff.json").c_str(), std::ios_base::app);
            }
            else if (last_loaded_cdl != "" && !dumpWholeROM) {
                file = std::ofstream(diff_filename.c_str(), std::ios_base::binary);
                copyOverOtherStatesDiffContents(&file);
            }
            else {
                file = std::ofstream(diff_filename.c_str(), std::ios_base::binary);
            }


            file << "\"" << bytesMapped << " (" <<  mapped_pc << "%%)\":{";
            differenceBetweenCurrentAndPreviousMemory(&file, dumpWholeROM,codeDataLogger::cdl_romsize,codeDataLogger::cdl_memchunk_,cdl_memchunk_rom_previous, &shouldWriteOutThePreviousBlockForROM, codeDataLogger::full_romchunk_);
            file << "},\n";
            file.close();

            if (!dumpWholeROM)
            {
             resetDiff();
             last_loaded_cdl = filename+".cdldiff.json";
             cdl_mapped_pc =mapped_pc;
            }



        }
    }

    unsigned char* setupPreviousMemoryForComparisonInDiff(unsigned char *previous_memory_pointer, unsigned char *new_memory_pointer, int memory_size) {
        if (previous_memory_pointer != nullptr) return previous_memory_pointer;
        previous_memory_pointer = new unsigned char[memory_size];
        memcpy(previous_memory_pointer,new_memory_pointer, memory_size);
        return previous_memory_pointer;
    }

    void setupAllPreviousMemoryChunksUsedForDiffing() {
        cdl_memchunk_rom_previous = setupPreviousMemoryForComparisonInDiff(cdl_memchunk_rom_previous, codeDataLogger::cdl_memchunk_, codeDataLogger::cdl_romsize);
        cdl_memchunk_vram_previous = setupPreviousMemoryForComparisonInDiff(cdl_memchunk_vram_previous, codeDataLogger::full_vram_, VRAM_SIZE);
        cdl_memchunk_tile_previous = setupPreviousMemoryForComparisonInDiff(cdl_memchunk_tile_previous, codeDataLogger::full_tile_map_, TILE_MAP_SIZE);
    }

    void calculateCDLStatistics(const std::string &filename, bool writeToFile, bool wholeROM) {
        if (cdl_memchunk_rom_previous == nullptr)
        {
            setupAllPreviousMemoryChunksUsedForDiffing();
            return;
        }
        unsigned int byteCounts[256];
        for (int i=0; i< 256; i++) {
            byteCounts[i] = 0;
        }
        int number_of_graphics_bytes = 0, number_of_bgmap_bytes=0, number_of_oam_bytes=0, number_of_sound_bytes=0;
        for (int i=0; i< codeDataLogger::cdl_romsize; i++) {
            char c =codeDataLogger::cdl_memchunk_[i];
            byteCounts[c]+=1;
            if (is_graphics(c)) {
                number_of_graphics_bytes++;
            }
            if (is_bgmap(c)) {
                number_of_bgmap_bytes++;
            }
            if (is_oam(c)) {
                number_of_oam_bytes++;
            }
            if (is_sound(c)) {
                number_of_sound_bytes++;
            }
        }

        //        for (int i=0; i< 256; i++) {
        //            unsigned int count =byteCounts[i];
        //            printf("\nByte: 0x%x count: %u",i,count);
        //        }

        float unmapped_pc = ( (float)byteCounts[0]/(float)codeDataLogger::cdl_romsize ) *100;
        float mapped_pc = 100-unmapped_pc;

        if (cdl_mapped_pc < mapped_pc) {

            printf("\n Mapped: %f %%",mapped_pc);
            printf("\n Not Mapped: %f%% (%d / %d)", unmapped_pc, byteCounts[0],codeDataLogger::cdl_romsize);


            printf("\nExecFirst (0x01): %d",byteCounts[1]);
            printf("\nExecOperand (0x02): %d",byteCounts[2]);
            printf("\nData (0x04): %d",byteCounts[4]);

    //       printf("\nData + ExecFirst (0x05): %d",byteCounts[5]);
    //       printf("\nData + ExecOperand (0x06): %d",byteCounts[6]);
            printf("\nGraphics bytes:: %d",number_of_graphics_bytes);
            printf("\nBGMap bytes:: %d",number_of_bgmap_bytes);
            printf("\nOAM bytes:: %d",number_of_oam_bytes);
            printf("\nSound bytes:: %d",number_of_sound_bytes);

        }


        if (writeToFile)
        {
            writeDiff(filename,mapped_pc, byteCounts[0], false);
            if (wholeROM)
            {
                std::cout << "Whole ROM\n";
                writeDiff(filename,mapped_pc, byteCounts[0], true);
            }
            //writeTileMap(filename);
        }
        //writeVRam();


    }

    bool loadCDL( const std::string &filename, const std::string &cdlpath) {
        has_loaded_state = true;
        last_loaded_cdl = cdlpath+".cdldiff.json";
        //        sourcecdlDiff = std::ifstream((cdlpath+".cdldiff.json").c_str(), std::ios::binary);
        std::ifstream file((cdlpath+".cdl").c_str(), std::ios_base::binary);
        file.ignore(15);
        char* gb = new char[3];

        std::string s = readString(&file, 2);
        printf("GB: %s",s.c_str());

        file.ignore(13);
        uint32_t number_of_blocks = readInteger32(&file);

        printf("number_of_blocks: %d pos:%d",number_of_blocks, (int)file.tellg());

        readBlock(&file,(char*)codeDataLogger::cdl_memchunk_);
        readBlock(&file,(char*)codeDataLogger::cdl_memchunk_workram);
        readBlock(&file,(char*)codeDataLogger::cdl_memchunk_cartram);
        setupAllPreviousMemoryChunksUsedForDiffing();

        //writeCDL(filename+"load");

        calculateCDLStatistics(filename,false,false);
        return true;
    }

    bool writeCDL( const std::string &filename, const std::string cdlpath) {
        std::ofstream file((cdlpath+".cdl").c_str(), std::ios_base::binary);
        if (file.fail())
            return false;

        file.put(0x0D);
        file.write("BIZHAWK-CDL-2",13);
        file.put(0x0F);
        file.write("GB",2);
        for (int i=0; i<13; i++)
        {file.put(0x20);}
        writeInteger(&file,NUM_OF_CDL_BLOCKS);

        file.put(0x03);
        file.write("ROM",3);
        writeBlock(&file,(const char *)codeDataLogger::cdl_memchunk_,codeDataLogger::cdl_romsize);

        file.put(0x04);
        file.write("WRAM",4);
        writeBlock(&file,(const char *)codeDataLogger::cdl_memchunk_workram,codeDataLogger::cdl_wramsize);

        file.put(0x07);
        file.write("CartRAM",7);
        writeBlock(&file,(const char *)codeDataLogger::cdl_memchunk_cartram,codeDataLogger::cdl_cartramsize);

        calculateCDLStatistics(cdlpath,true,true);

        return !file.fail();
    }

    bool writeExample() {
        std::ofstream file("example.txt", std::ios_base::binary);
        file.write("ALIM",4);
        return !file.fail();
    }


    unsigned char *cdl_memchunk_;
    unsigned char *cdl_memchunk_cartram;
    unsigned char *cdl_memchunk_workram;
    eCDLog_Flags last_data_access_was = eCDLog_Flags_None;
    int32_t previous_accessed_data_address=-1, previous_accessed_code_address=-1;
    //    addr:
    //    addrtype: e.g ROM, HRAM, WRAM, CartRAM
    //    flags:
    // Note: check memory_GBC.h for where these flags are set
    void default_cd_callback(int32_t addr, eCDLog_AddrType addrtype, eCDLog_Flags flags) {
        switch (addrtype)
        {
            case eCDLog_AddrType_ROM:
                cdl_memchunk_[addr] |= flags;
                if (is_data(flags)) {
                    previous_accessed_data_address=addr;
                } else if (is_code(flags)) {
                    previous_accessed_code_address = addr;
                }
                //                printf("CD Callback ROM %d", flags);
                break;
            case eCDLog_AddrType_HRAM:
                // Never used except in debugging as you don't need to map out the stack
                printf("HRAM %d", addr);
                break;
            case eCDLog_AddrType_WRAM:
                cdl_memchunk_workram[addr] |= flags;
                //printf("CD Callback WorkRAM %d", addr);
                break;
            case eCDLog_AddrType_TileRAM:
                cdl_memchunk_[previous_accessed_data_address] |= eCDLog_Flags_Graphics;
                cdl_memchunk_[previous_accessed_code_address] |= eCDLog_Flags_Graphics;
                break;
            case eCDLog_AddrType_BGMap:
                cdl_memchunk_[previous_accessed_data_address] |= eCDLog_Flags_BGMap;
                cdl_memchunk_[previous_accessed_code_address] |= eCDLog_Flags_BGMap;
                break;
            case eCDLog_AddrType_Sound:
                //std::cout << "Before " << is_sound(cdl_memchunk_[previous_accessed_data_address]) << "\n";
                cdl_memchunk_[previous_accessed_data_address] |= eCDLog_Flags_Sound;
                cdl_memchunk_[previous_accessed_code_address] |= eCDLog_Flags_Sound;
                //std::cout << "After " << is_sound(cdl_memchunk_[previous_accessed_data_address]) << "\n";
                break;

            case eCDLog_AddrType_OAM:
                // todo add is_oam function
                std::cout << "Before OAM " << is_oam(cdl_memchunk_[previous_accessed_data_address]) << "\n";
                cdl_memchunk_[previous_accessed_data_address] |= eCDLog_Flags_OAM;
                cdl_memchunk_[previous_accessed_code_address] |= eCDLog_Flags_OAM;
                std::cout << "After OAM " << is_oam(cdl_memchunk_[previous_accessed_data_address]) << "\n";
                break;
            case eCDLog_AddrType_CartRAM:
                cdl_memchunk_cartram[addr] |= flags;
                printf("CartRAM %d", addr);
                break;
            default:
                printf("Default addrtype %d", addrtype);
        }
    }
}
