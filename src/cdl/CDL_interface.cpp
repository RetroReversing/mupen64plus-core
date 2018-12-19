#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace std;

#include "CDL.hpp"
extern "C" {
#include "../main/rom.h"
#include "../device/r4300/tlb.h"
// TODO: need to log input and then call input.keyDown(keymod, keysym);

void main_state_save(int format, const char *filename);
void main_state_load(const char *filename);
void show_interface();
void corruptBytes(uint8_t* mem, uint32_t cartAddr, int times);
void load_jumps_from_json() ;
void saveJsonToFile();
void write_rom_mapping();
void cdl_log_pif_ram(uint32_t address, uint32_t* value);

extern int   l_CurrentFrame;

// 
// # Variables
// 
int corrupt_start =  0xb2b77c;
int corrupt_end = 0xb2b77c;
int difference = corrupt_end-corrupt_start;
std::map<uint, string> registerChanges;
json fileConfig;
std::map<unsigned int,char> jumps;
std::map<unsigned int, uint32_t> jump_data;
std::map<int,int> dram_reads;
std::map<int,int> dram_writes;
std::map<int,int> rsp_reads;
std::map<int,int> rdram_reads;
std::map<int,bool> offsetHasAssembly;
string last_reversed_address = "";
bool should_reverse_jumps = false;
int frame_last_reversed = 0;
int time_last_reversed = 0;

void cdl_keyevents(int keysym, int keymod) {
    printf("event_sdl_keydown frame:%d key:%d modifier:%d \n", l_CurrentFrame, keysym, keymod);
    should_reverse_jumps = false;
    // S key
    if (keysym == 115) {
        printf("Lets save! \n");
        main_state_save(0, NULL);
    }
    // L Key
    if (keysym == 108) {
        printf("Lets load! \n");
        main_state_load(NULL);
    }
    // Z Key
    if (keysym == 122) {
        write_rom_mapping();
        should_reverse_jumps = true;
        time_last_reversed = time(0);
        // show_interface();
    }
}

bool createdCartBackup = false;
void backupCart() {
    std::cout << "TODO: backup";
    createdCartBackup = true;
}
void resetCart() {
    std::cout << "TODO: reset";
}

void readJsonFromFile() {
    string filename = ROM_PARAMS.headername;
    filename += ".json";
    // read a JSON file
    std::ifstream i(filename);
    if (i.good()) {
        i >> fileConfig;
    }
    
    if (fileConfig.find("mappings") == fileConfig.end()) {
                fileConfig["mappings"] = R"(
                {                }
                )"_json;
    }
    if (fileConfig.find("jumps") == fileConfig.end()) {
                fileConfig["jumps"] = R"(
                {
                }
                )"_json;
    }
    if (fileConfig.find("tlb") == fileConfig.end()) {
                fileConfig["tlb"] = R"({})"_json;
    }
    if (fileConfig.find("rsp_reads") == fileConfig.end()) {
                fileConfig["rsp_reads"] = R"({})"_json;
    }
    if (fileConfig.find("rdram_reads") == fileConfig.end())
                fileConfig["rdram_reads"] = R"({})"_json;
    if (fileConfig.find("reversed_jumps") == fileConfig.end())
                fileConfig["reversed_jumps"] = R"({})"_json;

}
void saveJsonToFile() {
    string filename = ROM_PARAMS.headername;
    filename += ".json";
    // write prettified JSON to another file
    std::ofstream o(filename);
    o << fileConfig.dump(4) << std::endl;
}

void show_interface() {
    int answer;
    std::cout << "1) Reset ROM 2) Change corrupt number ";
    std::cin >> std::hex >> answer;
    if (answer == 1) {
        std::cout << "Resetting ROM";
        resetCart();
    }
    else {
        std::cout << "Unknown command";
    }
    printf("Answer: %d \n", answer);
}

void corrupt_if_in_range(uint8_t* mem, uint32_t proper_cart_address) {
    // if (proper_cart_address >= corrupt_start && proper_cart_address <= corrupt_end) { //l_CurrentFrame == 0x478 && length == 0x04) { //} proper_cart_address == 0xb4015c) {
    //     printf("save_state_before\n");
    //     main_state_save(0, "before_corruption");
    //     printBytes(mem, proper_cart_address);
    //     printf("MODIFIED IT!! %#08x\n\n\n", mem[proper_cart_address+1]);
    //     corruptBytes(mem, proper_cart_address, 10);
    //     printBytes(mem, proper_cart_address);
    // }
}

void corruptBytes(uint8_t* mem, uint32_t cartAddr, int times) {
    if (times>difference) {
        times=difference/4;
    }
    srand(time(NULL)); 
    printf("Corrupt Start: %d End: %d Difference: %d \n", corrupt_start, corrupt_end, difference);
    int randomNewValue = rand() % 0xFF;
    for (int i=0; i<=times; i++) {
        int randomOffset = rand() % difference;
        int currentOffset = randomOffset;
        printf("Offset: %d OldValue: %#08x NewValue: %#08x \n", currentOffset, mem[cartAddr+currentOffset], randomNewValue);
        mem[cartAddr+currentOffset] = randomNewValue;
    }
}

void cdl_log_opcode(uint32_t program_counter, uint32_t* op_address) {
    // if (jumps[program_counter]>0) {
        jump_data[program_counter] = *op_address;
        // fileConfig["jumps_data"][program_counter] =  n2hexstr(*op_address);
        // std::cout << "Jump instr: " << std::hex << *op_address << " PC:" << program_counter << " :" << jump_data[program_counter] << "\n";
    // }
}

string get_header_ascii(uint8_t* mem, uint32_t proper_cart_address) {
    std::stringstream sstream;
    for (int i=3; i>=0; i--) {
        char c = mem[proper_cart_address+i];
        if (!isalpha(c)) { break; }
        if (c == '!' || c == '^' || c == '+' || c == '/' || c == ':' || c == '%'|| c == '"' || c == '#') { break; }
        sstream << c;
    }
    return sstream.str();
}



string to_hex(int my_integer) {
    std::stringstream sstream;
    sstream << std::hex << my_integer;
    return sstream.str();
}

struct cdl_tlb {
  unsigned int start;
  unsigned int end;
  unsigned int rom_offset;
};
vector<cdl_tlb> tlbs;

uint32_t map_assembly_offset_to_rom_offset(uint32_t assembly_offset) {
    // or if its in KSEG0/1

    for (auto& t: tlbs) {
        if (assembly_offset>=t.start && assembly_offset <=t.end) {
            // std::cout<< "T:" << t.start << " " << t.end << " " << t.rom_offset << "\n";
            return t.rom_offset + (assembly_offset-t.start);
        }
    }
    // std::cout << "Unmapped: " << std::hex << assembly_offset << "\n";
    return assembly_offset;
}

// these are all the data regions
// for assembly regions check out the tlb
void create_n64_split_regions(uint8_t* mem, uint32_t proper_cart_address, uint32_t length, uint32_t dram_addr) {
    std::stringstream sstream, header;
    string proper_cart_address_str = n2hexstr(proper_cart_address);
    string ascii_header = get_header_ascii(mem, proper_cart_address);
    // printf("WTF: %#08x \n",mem[proper_cart_address]);
    header << " header: " <<  ascii_header << " 0x" << std::hex << (mem[proper_cart_address+3]+0) << (mem[proper_cart_address+2]+0) << (mem[proper_cart_address+1]+0) << (mem[proper_cart_address]+0);
    // header << " ascii: " << mem[proper_cart_address+3] << mem[proper_cart_address+2] << mem[proper_cart_address+1] << mem[proper_cart_address];
    sstream << "  - [0x" << std::hex << proper_cart_address << ", 0x"<< (proper_cart_address+length) << ", \"bin\",    \"_" << ascii_header << "_" << proper_cart_address << "_len_"<< length << "\"] # , 0x" << (dram_addr) << "] frame:0x" << n2hexstr(l_CurrentFrame) << header.str();
    std::string mapping = sstream.str();
    registerChanges[proper_cart_address] = ""+mapping;
    fileConfig["mappings"][proper_cart_address_str] = mapping;
    // fileConfig["mappings"][proper_cart_address_str] = R"({})"_json;
    // fileConfig["mappings"][proper_cart_address_str]["start"] = proper_cart_address;
    // fileConfig["mappings"][proper_cart_address_str]["end"] = proper_cart_address+length;

    printf("dma_pi_write frame:%d %s\n", l_CurrentFrame, mapping.c_str());
}

void log_dma_write(uint8_t* mem, uint32_t proper_cart_address, uint32_t cart_addr, uint32_t length, uint32_t dram_addr) {
    if (registerChanges.find(proper_cart_address) != registerChanges.end() ) 
        return;
    string proper_cart_address_str = n2hexstr(proper_cart_address);
    bool isInJson = fileConfig["mappings"].find(proper_cart_address_str) != fileConfig["mappings"].end();
    if (isInJson) return;
    

    if (!createdCartBackup) {
        backupCart();
        readJsonFromFile();
        load_jumps_from_json();
    }
    create_n64_split_regions(mem, proper_cart_address, length, dram_addr);
}

void save_jumps_to_json() {
    printf("save_jumps_from_json \n");
    for(map<unsigned int, char>::iterator it = jumps.begin(); it != jumps.end(); ++it) {
        unsigned int addr = it->first;//  >> 12;
        offsetHasAssembly[addr] = true;
        string jump_target_str = n2hexstr(it->first);
        // std::cout << addr  << " : " << (0+jumps[addr]) << "\n";
        fileConfig["jumps"][jump_target_str] = (it->second+0);
        fileConfig["jumps_rom"][jump_target_str] =  n2hexstr(map_assembly_offset_to_rom_offset(it->first));
        fileConfig["jumps_data"][jump_target_str] =  n2hexstr(jump_data[it->first]);
        // std::cout << "Instruction data:" << std::hex << jump_data[addr] << "offset:" << map_assembly_offset_to_rom_offset(it->first) << "\n";
    }
}

void load_jumps_from_json() {
    printf("load_jumps_from_json \n");

    for (json::iterator it = fileConfig["jumps"].begin(); it != fileConfig["jumps"].end(); ++it) {
        unsigned int address = hex_to_int(it.key());
        int value = it.value();
        jumps[address] = value; //stoi(value);
        // std::cout << address  << " : " << (0+jumps[address]) << "\n";
    }
}

void load_tbls_from_json() {
    printf("load_tbls_from_json \n");
// todo
    for (json::iterator it = fileConfig["tlb"].begin(); it != fileConfig["tlb"].end(); ++it) {
        unsigned int address = hex_to_int(it.key());
        int value = it.value();

        auto t = cdl_tlb();
        // t.start=start;
        // t.end = end;
        // t.rom_offset = phys;

        tlbs.push_back(t);
        // jumps[address] = value; //stoi(value);
        std::cout << address  << " : " << (0+jumps[address]) << "\n";
    }
}

void save_it_to_json(map<int, int>::iterator it, map<int, int>::iterator end, string keyName) {
    for(; it != end; ++it) {
        string jump_target_str = n2hexstr(it->first);
        string value_at_address = n2hexstr(it->second);
        if (fileConfig[keyName].find(jump_target_str) != fileConfig[keyName].end() ) {
            string old_value = fileConfig[keyName][jump_target_str];
            if (old_value!=value_at_address) {
                printf("%s: old:%s new:%s \n", keyName.c_str(), old_value.c_str(), value_at_address.c_str());
            }
        }
        fileConfig[keyName][jump_target_str] = value_at_address;
    }
    
}

void save_dram_rw_to_json() {
    for(map<int, int>::iterator it = rdram_reads.begin(); it != rdram_reads.end(); ++it) {
        string jump_target_str = n2hexstr(it->first);
        string value_at_address = n2hexstr(it->second);
        if (fileConfig["rdram_reads"].find(jump_target_str) != fileConfig["rdram_reads"].end() ) {
            string old_value = fileConfig["rdram_reads"][jump_target_str];
            if (old_value!=value_at_address) {
                printf("rdram_reads: old:%s new:%s \n", old_value.c_str(), value_at_address.c_str());
            }
        }
        fileConfig["rdram_reads"][jump_target_str] = value_at_address;
    }
    save_it_to_json(rsp_reads.begin(), rsp_reads.end(), "rsp_reads");
}

void resetReversing() {
    time_last_reversed = time(0);
    last_reversed_address="";
}


void write_rom_mapping() {
    resetReversing();

    save_jumps_to_json();
    save_dram_rw_to_json();
    saveJsonToFile();
    printf("ROM_PARAMS.headername: %s \n", ROM_PARAMS.headername);
    string filename = ROM_PARAMS.headername;
    filename += ".config.yaml";
    ofstream file(filename, std::ios_base::binary);
    file << "# ROM splitter configuration file\n";
    file << "name: \"";
    file << ROM_SETTINGS.goodname;
    file << "\"\n";
    file <<"# checksums from ROM header offsets 0x10 and 0x14\n";
    file <<"# used for auto configuration detection\n";
    file <<"checksum1: 0x";
    file << std::hex << ROM_HEADER.CRC1;
    file <<"\nchecksum2: 0x";
    file << std::hex << ROM_HEADER.CRC2;
    file <<"\n# base filename used for outputs - (please, no spaces)\n";
    file <<"basename: \"";
    file << alphabetic_only_name(ROM_PARAMS.headername, 21);
    file << "\"\n";
    file <<"ranges:\n";
    file <<"  # start,  end,      type,     label\n";
    file <<"  - [0x000000, 0x000040, \"header\", \"header\"]\n";
    file <<"  - [0x000040, 0x000B70, \"asm\",    \"boot\"]\n";
    file <<"  - [0x000B70, 0x001000, \"bin\",    \"bootbin\"]\n";
    //
    // Write out 
    //
    //for(map<int, char*>::iterator it = mapOfInstructions.begin(); it != mapOfInstructions.end(); ++it) {
    // for (json::iterator it = fileConfig["mappings"].begin(); it != fileConfig["mappings"].end(); ++it) {
    for (auto& it : fileConfig["mappings"]) {
        string myStr = it;
        file << myStr << "\n";
    }

    


}

int reverse_jump(int take_jump, uint32_t jump_target) {
    time_t now = time(0);
    string key = n2hexstr(jump_target);          
    printf("Reversing jump %#08x %d \n", jump_target, jumps[jump_target]);
    take_jump = !take_jump;
    time_last_reversed = now;
    frame_last_reversed=l_CurrentFrame;
    last_reversed_address = key;
    fileConfig["reversed_jumps"][key] = jumps[jump_target];
    write_rom_mapping();
    return take_jump;
}

void cdl_log_jump_always(int take_jump, uint32_t jump_target) {
    // if (jumps[jump_target] >3) return;
    jumps[jump_target] = 0x04;
}

unsigned int find_first_non_executed_jump() {
    for(map<unsigned int, char>::iterator it = jumps.begin(); it != jumps.end(); ++it) {
        if ((it->second+0) <3) {
            return it->first;
        }
    }
    return -1;
}

bool is_physical_data_mapped_using_tlb() {
    // todo
    return false;
}

bool does_section_contain_assembly() {
    // todo: use offsetHasAssembly[addr] = true;
    // fileConfig["mappings"] contains start and end
    for (json::iterator it = fileConfig["mappings"].begin(); it != fileConfig["mappings"].end(); ++it) {
        unsigned int address = hex_to_int(it.key());
        int value = it.value();
        std::cout << address  << " : " << value << "\n";
    }
    return false;
}

// void cdl_log_assembly_location(uint32_t address_of_known_assembly) {
//     printf("cdl_log_assembly_location %#08x \n", address_of_known_assembly);
//     // now need to get tlb information
//     // lets loop through all the tlb's
//     // todo
    
// }

int cdl_log_jump(int take_jump, uint32_t jump_target) {
    // if (jumps[jump_target] < 3 && take_jump) {
    //     cdl_log_assembly_location(jump_target);
    // }

    if (should_reverse_jumps)
    {
        time_t now = time(0);
        if (jumps[jump_target] < 3) {
            // should_reverse_jumps=false;
            if ( now-time_last_reversed > 2) { // l_CurrentFrame-frame_last_reversed >(10*5) ||
                take_jump = reverse_jump(take_jump, jump_target);               
            }
        } else if (now-time_last_reversed > 15) {
            printf("Stuck fixing %d\n", find_first_non_executed_jump());
            take_jump=!take_jump;
            main_state_load(NULL);
            // we are stuck so lets load
        }
    }
    if (take_jump) {
        jumps[jump_target] |= 1UL << 0;
    }
    else {
        jumps[jump_target] |= 1UL << 1;
    }
    return take_jump;
}

void save_table_mapping(int entry, unsigned int phys, unsigned int start, unsigned int end, bool isOdd) {
    
    //printf("tlb_map:%d ODD Start:%#08x End:%#08x Phys:%#08x \n",entry, e->start_odd, e->end_odd, e->phys_odd);
        unsigned int length = end-start;

        auto t = cdl_tlb();
        t.start=start;
        t.end = end;
        t.rom_offset = phys;

        tlbs.push_back(t);

        string key = "";
        key+="[0x";
        key+=n2hexstr(phys);
        key+=", 0x";
        key+=n2hexstr(phys+length);
        key+="] Virtual: 0x";
        key+=n2hexstr(start);
        key+=" to 0x";
        key+=n2hexstr(end);
        if (isOdd)
        key+=" Odd";
        else
        key+=" Even";

        string value = "Entry:";
        value += to_string(entry);
        // value += " Frame:0x";
        value += n2hexstr(l_CurrentFrame);

        bool isInJson = fileConfig["tlb"].find(key) != fileConfig["tlb"].end();
        if (isInJson) {
            string original = fileConfig["tlb"][key];
            bool isSameValue = (strcmp(original.c_str(), value.c_str()) == 0);
            if (isSameValue) return;
            // printf("isSameValue:%d \noriginal:%s \nnew:%s\n", isSameValue, original.c_str(), value.c_str());
            return; // don't replace the original value as it is useful to match frame numbers to the mappings
        }
        fileConfig["tlb"][key] = value;
        printf("TLB %s\n", value.c_str());
}

// ASID: The asid argument specifies an address space identifier that makes the mappings valid only when a specific address space identifier register is loaded. (See osSetTLBASID.) A value of -1 for asid specifies a global mapping that is always valid.
// The CPU TLB consists of 32 entries, which provide mapping to 32 odd/even physical page pairs (64 total pages).
// Where is Page Size PM?
void log_tlb_entry(const struct tlb_entry* e, size_t entry) {
    // I think g is 1 when either odd or even is used
    // printf("tlb_map:%d MISC mask:%d vpn2:%#08x g:%d r:%d asid:%d \n",entry, e->mask, e->vpn2, e->g, e->r, e->asid);
    if (e->v_even) {
        save_table_mapping(entry, e->phys_even, e->start_even, e->end_even, false);
    }
    if (e->v_odd) {
        save_table_mapping(entry, e->phys_odd, e->start_odd, e->end_odd, true);
    }
}

void cdl_log_dram_read(uint32_t address) {
    
}
void cdl_log_dram_write(uint32_t address, uint32_t value, uint32_t mask) {
    
}

void cdl_log_rsp_mem(uint32_t address, uint32_t* mem,int isBootRom) {
    if (isBootRom) return;
    rsp_reads[address] = (uint32_t)*mem;
}
void cdl_log_rdram(uint32_t address, uint32_t* mem,int isBootRom) {
    //printf("RDRAM %#08x \n", address);
    if (isBootRom) return;
    rdram_reads[address] = (uint32_t)*mem;
}
void cdl_log_mm_cart_rom(uint32_t address,int isBootRom) {
    printf("Cart ROM %#08x \n", address);
}
void cdl_log_mm_cart_rom_pif(uint32_t address,int isBootRom) {
    printf("PIF? %#08x \n", address);
}

void cdl_log_pif_ram(uint32_t address, uint32_t* value) {
    printf("Game was reset? \n");
    if (should_reverse_jumps) {
        // should_reverse_jumps = false;
        fileConfig["bad_jumps"][last_reversed_address] = "reset";
        main_state_load(NULL);
        write_rom_mapping();
    }
}

void cdl_log_opcode_error() {
    printf("Very bad opcode, caused crash! \n");
    fileConfig["bad_jumps"][last_reversed_address] = "crash";
    main_state_load(NULL);
}

} // end extern C