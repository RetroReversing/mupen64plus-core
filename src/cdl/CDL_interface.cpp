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

json fileConfig;
json libultra_signatures;
#define USE_CDL 1;

extern "C" {
#include "../main/rom.h"
#include "../device/r4300/tlb.h"
// TODO: need to log input and then call input.keyDown(keymod, keysym);

void main_state_save(int format, const char *filename);
void main_state_load(const char *filename);
void show_interface();
void corruptBytes(uint8_t* mem, uint32_t cartAddr, int times);
void saveJsonToFile();
void write_rom_mapping();
void cdl_log_pif_ram(uint32_t address, uint32_t* value);
void find_asm_sections();
uint32_t map_assembly_offset_to_rom_offset(uint32_t assembly_offset, uint32_t tlb_mapped_addr);

extern int   l_CurrentFrame;

// 
// # Variables
// 
int corrupt_start =  0xb2b77c;
int corrupt_end = 0xb2b77c;
int difference = corrupt_end-corrupt_start;
std::map<uint32_t,char> jumps;
std::map<uint32_t, uint8_t*> jump_data;
std::map<uint32_t,uint32_t> rsp_reads;
std::map<uint32_t,uint32_t> rdram_reads;
std::map<uint32_t,bool> offsetHasAssembly;
string last_reversed_address = "";
bool should_reverse_jumps = false;
int frame_last_reversed = 0;
int time_last_reversed = 0;
// uint32_t previous_function = 0;
 std::vector<uint32_t> function_stack;

std::map<string, string> function_signatures;

std::map<uint32_t, cdl_dram_cart_map> audio_samples;
std::map<uint32_t, cdl_dram_cart_map> cart_rom_dma_writes;
std::map<uint32_t, cdl_dram_cart_map> dma_sp_writes;
std::map<uint32_t, cdl_labels> labels;
std::map<uint32_t, cdl_jump_return> jump_returns;
std::map<uint32_t,cdl_tlb> tlbs;
std::map<uint32_t,cdl_dma> dmas = std::map<uint32_t,cdl_dma>();

void cdl_keyevents(int keysym, int keymod) {
    #ifndef USE_CDL
        return;
    #endif
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

void readLibUltraSignatures() {
    std::ifstream i("libultra.json");
    if (i.good()) {
        i >> libultra_signatures;
    } 
    if (libultra_signatures.find("function_signatures") == libultra_signatures.end()) {
            libultra_signatures["function_signatures"] = R"([])"_json;
    }
}

void readJsonFromFile() {
    readLibUltraSignatures();
    string filename = ROM_PARAMS.headername;
    filename += ".json";
    // read a JSON file
    std::ifstream i(filename);
    if (i.good()) {
        i >> fileConfig;
    }
    if (fileConfig.find("jumps") == fileConfig.end()) {
                fileConfig["jumps"] = R"([])"_json;
    }
    if (fileConfig.find("tlbs") == fileConfig.end()) {
                fileConfig["tlbs"] = R"([])"_json;
    }
    if (fileConfig.find("dmas") == fileConfig.end()) {
                fileConfig["dmas"] = R"([])"_json;
    }
    if (fileConfig.find("rsp_reads") == fileConfig.end()) {
                fileConfig["rsp_reads"] = R"([])"_json;
    }
    if (fileConfig.find("rdram_reads") == fileConfig.end())
                fileConfig["rdram_reads"] = R"([])"_json;
    if (fileConfig.find("reversed_jumps") == fileConfig.end())
                fileConfig["reversed_jumps"] = R"({})"_json;
    if (fileConfig.find("labels") == fileConfig.end())
                fileConfig["labels"] = R"([])"_json;
    if (fileConfig.find("jump_returns") == fileConfig.end())
                fileConfig["jump_returns"] = R"([])"_json;

    jumps = fileConfig["jumps"].get< std::map<uint32_t,char> >();
    tlbs = fileConfig["tlbs"].get< std::map<uint32_t,cdl_tlb> >();
    dmas = fileConfig["dmas"].get< std::map<uint32_t,cdl_dma> >();
    rsp_reads = fileConfig["rsp_reads"].get< std::map<uint32_t,uint32_t> >();
    rdram_reads = fileConfig["rdram_reads"].get< std::map<uint32_t,uint32_t> >();
    labels = fileConfig["labels"].get< std::map<uint32_t,cdl_labels> >();
    jump_returns = fileConfig["jump_returns"].get< std::map<uint32_t,cdl_jump_return> >();
}

void saveJsonToFile() {
    string filename = ROM_PARAMS.headername;
    filename += ".json";
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
    #ifndef USE_CDL
        return;
    #endif
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

void cdl_log_opcode(uint32_t program_counter, uint8_t* op_address) {
        // jump_data[program_counter] = op_address;
}

uint32_t map_assembly_offset_to_rom_offset(uint32_t assembly_offset, uint32_t tlb_mapped_addr) {
    // or if its in KSEG0/1
    if (assembly_offset >= 0x80000000) {
        uint32_t mapped_offset = assembly_offset & UINT32_C(0x1ffffffc);
        // std::cout << "todo:" << std::hex << assembly_offset << "\n";
        return map_assembly_offset_to_rom_offset(mapped_offset, assembly_offset);
    }

    for(auto it = tlbs.begin(); it != tlbs.end(); ++it) {
        auto t = it->second;
        if (assembly_offset>=t.start && assembly_offset <=t.end) {
            uint32_t mapped_offset = t.rom_offset + (assembly_offset-t.start);
            return map_assembly_offset_to_rom_offset(mapped_offset, assembly_offset);
        }
    }
    for(auto it = dmas.begin(); it != dmas.end(); ++it) {
        auto& t = it->second;
        if (assembly_offset>=t.dram_start && assembly_offset <=t.dram_end) {
            uint32_t mapped_offset = t.rom_start + (assembly_offset-t.dram_start);
            t.is_assembly = true;
            t.tbl_mapped_addr = tlb_mapped_addr;
            // DMA is likely the actual value in rom
            return mapped_offset;
        }
    }
    // std::cout << "Not in dmas:" << std::hex << assembly_offset << "\n";
    // std::cout << "Unmapped: " << std::hex << assembly_offset << "\n";
    return assembly_offset;
}

// these are all the data regions
// for assembly regions check out the tlb
string create_n64_split_regions(cdl_dma d) { //uint8_t* header_bytes, uint32_t proper_cart_address, uint32_t length, uint32_t dram_addr, uint32_t frame,  bool is_assembly, uint32_t tbl_mapped_addr) {
    uint8_t* header_bytes = (uint8_t*)&d.header;
    std::stringstream sstream, header;
    string region_type = "bin";
    if (d.is_assembly) {
        region_type="asm";
    }
    string proper_cart_address_str = n2hexstr(d.rom_start);
    string ascii_header = d.ascii_header;
    header << " header: " <<  ascii_header << " 0x" << std::hex << (header_bytes[3]+0) << (header_bytes[2]+0) << (header_bytes[1]+0) << (header_bytes[0]+0);
    sstream << "  - [0x" << std::hex << d.rom_start << ", 0x"<< (d.rom_start+d.length);
    sstream << ", \"" << region_type << "\",    \"_" << ascii_header << "_" << d.rom_start << "_len_"<< d.length;
    sstream << "\"] # , 0x" << n2hexstr(d.dram_start) << "] frame:0x" << n2hexstr(d.frame) << header.str();
    //sstream << " Func:" << d.func_addr;
    
    if (d.tbl_mapped_addr>0) {
        sstream << "Tbl mapped:"<<d.tbl_mapped_addr;
    }

    std::string mapping = sstream.str();
    return mapping;
}

void log_dma_write(uint8_t* mem, uint32_t proper_cart_address, uint32_t cart_addr, uint32_t length, uint32_t dram_addr) {
    if (dmas.find(proper_cart_address) != dmas.end() ) 
        return;

    uint32_t current_function = 0;
     // we know this function name: osPiRawStartDma
     if (function_stack.size() > 0) {
         current_function = function_stack.back();
         if (labels.find(current_function) != labels.end()) {
            labels[current_function].func_name = "osPiRawStartDma";
        }
     }

    auto t = cdl_dma();
    t.dram_start=dram_addr;
    t.dram_end = dram_addr+length;
    t.rom_start = proper_cart_address;
    t.rom_end = proper_cart_address+length;
    t.length = length;
    t.ascii_header = get_header_ascii(mem, proper_cart_address);
    t.header = mem[proper_cart_address+3];
    t.frame = l_CurrentFrame;

    if (function_stack.size() > 0 && labels.find(current_function) != labels.end()) {
        t.func_addr = labels[current_function].func_name;
    }

    dmas[proper_cart_address] = t;

    std::cout << "DMA: Dram:0x" << std::hex << t.dram_start << "->0x" << t.dram_end << " Length:0x" << t.length << " " << t.ascii_header << " Stack:" << function_stack.size() << "\n";
    
}


void save_dram_rw_to_json() {
    // Note if you save it here you might want to also update readJsonFromFile()
    fileConfig["rsp_reads"] = rsp_reads;
    fileConfig["rdram_reads"] = rdram_reads;
    fileConfig["tlbs"] = tlbs;
    fileConfig["dmas"] = dmas;
    fileConfig["jumps"] = jumps;
    //fileConfig["jump_data"] = jump_data;
    fileConfig["audio_samples"] = audio_samples;
    fileConfig["cart_rom_dma_writes"] = cart_rom_dma_writes;
    fileConfig["dma_sp_writes"] = dma_sp_writes;
    fileConfig["labels"] = labels;
    fileConfig["function_signatures"] = function_signatures;
}

void resetReversing() {
    time_last_reversed = time(0);
    last_reversed_address="";
}

void save_cdl_files() {
    resetReversing();
    find_asm_sections();
    save_dram_rw_to_json();
    saveJsonToFile();
}


void write_rom_mapping() {
    save_cdl_files();
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
    for (auto& it : dmas) {
        auto t = it.second;
        file << create_n64_split_regions(t) << "\n";
    }

    file <<"# Labels for functions or data memory addresses\n";
    file <<"# All label addresses are RAM addresses\n";
    file <<"# Order does not matter\n";
    file <<"labels:\n";
    int8_t* entryPoint = (int8_t*)(void*)&ROM_HEADER.PC;
    file << "   - [0x" << (entryPoint[0]+0) << (entryPoint[1]+0) << (entryPoint[2] +0)<< (entryPoint[3]+0) <<", \"EntryPoint\"]\n";
    for (auto& it : labels) {
        auto t = it.second;
        file << "   - [0x" << t.func_offset <<", \"" <<  t.func_name << "\"]\n";
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

void cdl_log_jump_always(int take_jump, uint32_t jump_target, uint8_t* jump_target_memory) {
    uint32_t previous_function_backup = function_stack.back();
    function_stack.push_back(jump_target);

    if (jumps[jump_target] >3) return;
    jumps[jump_target] = 0x04;

    if (labels.find(jump_target) != labels.end() ) 
        return;
    auto t = cdl_labels();
    string jump_target_str = n2hexstr(jump_target);
    t.func_offset = jump_target_str;
    t.caller_offset = n2hexstr(previous_function_backup);
    t.func_name = "func_"+jump_target_str;
    t.func_stack = function_stack.size();
    labels[jump_target] = t;
    jump_data[jump_target] = jump_target_memory;
}
void cdl_log_jump_return(int take_jump, uint32_t jump_target, uint32_t pc) {
    uint32_t previous_function_backup = function_stack.back();
    if (function_stack.size()>0) {
        previous_function_backup = function_stack.back();
        function_stack.pop_back();
    }


    if (jumps[jump_target] >3) return;
    jumps[jump_target] = 0x04;

    if (jump_returns.find(previous_function_backup) != jump_returns.end()) 
        {
            return;
        }
    auto t = cdl_jump_return();
    string jump_target_str = n2hexstr(jump_target);
    t.return_offset = pc;
    t.func_offset = previous_function_backup;
    t.caller_offset = jump_target;
    jump_returns[previous_function_backup] = t;

    uint64_t length = pc-previous_function_backup;
    labels[previous_function_backup].return_offset_from_start = length;
    if (jump_data.find(previous_function_backup) != jump_data.end()) {
        uint64_t byte_len = length;
        if (byte_len > 0xFF) {
            byte_len = 0xFF;
        }
        string bytes = printBytesToStr(jump_data[previous_function_backup], byte_len)+"_"+n2hexstr(length);
        labels[previous_function_backup].function_bytes = bytes;

        // if it is a libultra function then lets name it
        if (libultra_signatures["function_signatures"].find(bytes) != libultra_signatures["function_signatures"].end()) {
            std::cout << "In libultra:" <<  bytes << "\n";
            labels[previous_function_backup].func_name = libultra_signatures["function_signatures"][bytes];
        }
        else {
            std::cout << "Not in lib_ultra " << bytes << "\n";
        }

        if (function_signatures.find(bytes) == function_signatures.end()) {
            function_signatures[bytes] = labels[previous_function_backup].func_name;
        } else {
            function_signatures[bytes] = "Multiple functions";
            std::cout << "WHY:" << *jump_data[previous_function_backup] << " len:" << length << " pc:0x"<< pc << " - 0x" << previous_function_backup << "\n";
        }
    }
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

int cdl_log_jump(int take_jump, uint32_t jump_target) {

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

void save_table_mapping(int entry, uint32_t phys, uint32_t start,uint32_t end, bool isOdd) {
    
    //printf("tlb_map:%d ODD Start:%#08x End:%#08x Phys:%#08x \n",entry, e->start_odd, e->end_odd, e->phys_odd);
        uint32_t length = end-start;

        auto t = cdl_tlb();
        t.start=start;
        t.end = end;
        t.rom_offset = phys;
        tlbs[phys]=t;

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
    #ifndef USE_CDL
        return;
    #endif
    printf("Game was reset? \n");
    if (!createdCartBackup) {
        backupCart();
        readJsonFromFile();
        function_stack.push_back(0);
    }
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

void find_asm_sections() {
    printf("finding asm in sections \n");
    for(map<unsigned int, char>::iterator it = jumps.begin(); it != jumps.end(); ++it) {
        string jump_target_str = n2hexstr(it->first);
        fileConfig["jumps_rom"][jump_target_str] =  n2hexstr(map_assembly_offset_to_rom_offset(it->first,0));
    }
}

void cdl_log_audio_sample(uint32_t saved_ai_dram, uint32_t saved_ai_length) {
    if (audio_samples.find(saved_ai_dram) != audio_samples.end() ) 
        return;
    auto t = cdl_dram_cart_map();
    t.dram_offset = n2hexstr(saved_ai_dram);
    t.rom_offset = n2hexstr(saved_ai_length);
    audio_samples[saved_ai_dram] = t;
    printf("audio_plugin_push_samples AI_DRAM_ADDR_REG:%#08x length:%#08x\n", saved_ai_dram, saved_ai_length);
}

void cdl_log_cart_rom_dma_write(uint32_t dram_addr, uint32_t cart_addr, uint32_t length) {
    //cart_addr-=0x10000000;
    if (cart_rom_dma_writes.find(cart_addr) != cart_rom_dma_writes.end() ) 
        return;
    auto t = cdl_dram_cart_map();
    t.dram_offset = n2hexstr(dram_addr);
    t.rom_offset = n2hexstr(cart_addr);
    cart_rom_dma_writes[cart_addr] = t;
    printf("cart_rom_dma_write: dram_addr:%#008x cart_addr:%#008x length:%#008x\n", dram_addr, cart_addr, length);
}

void cdl_log_dma_sp_write(uint32_t spmemaddr, uint32_t dramaddr, uint32_t length, unsigned char *dram) {
    #ifndef USE_CDL
        return;
    #endif
    if (dma_sp_writes.find(dramaddr) != dma_sp_writes.end() ) 
        return;
    auto t = cdl_dram_cart_map();
    t.dram_offset = n2hexstr(dramaddr);
    t.rom_offset = n2hexstr(spmemaddr);
    dma_sp_writes[dramaddr] = t;
    // FrameBuffer RSP info
    printWords(dram, dramaddr, length);
    printf("FB: dma_sp_write SPMemAddr:%#08x Dramaddr:%#08x length:%#08x  \n", spmemaddr, dramaddr, length);
}

} // end extern C