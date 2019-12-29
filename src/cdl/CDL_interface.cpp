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

extern json fileConfig;
extern json reConfig;
json libultra_signatures;
json linker_map_file;
#define USE_CDL 1;

extern "C" {
#include "../main/rom.h"
#include "../device/r4300/tlb.h"
#include "../device/r4300/r4300_core.h"
#include "../device/memory/memory.h"
// TODO: need to log input and then call input.keyDown(keymod, keysym);

void main_state_save(int format, const char *filename);
void main_state_load(const char *filename);
void show_interface();
void corruptBytes(uint8_t* mem, uint32_t cartAddr, int times);
void saveJsonToFile();
void write_rom_mapping();
void cdl_log_pif_ram(uint32_t address, uint32_t* value);
void find_asm_sections();
void find_audio_sections();
void find_audio_functions();
bool isAddressCartROM(u_int32_t address);
void add_tag_to_function(string tag, uint32_t labelAddr);
uint32_t map_assembly_offset_to_rom_offset(uint32_t assembly_offset, uint32_t tlb_mapped_addr);
string print_function_stack_trace();
bool is_auto_generated_function_name(string func_name);

extern int   l_CurrentFrame;

// 
// # Variables
// 
int corrupt_start =  0xb2b77c;
int corrupt_end = 0xb2b77c;
int difference = corrupt_end-corrupt_start;
extern std::map<uint32_t,string> memory_to_log;
extern std::map<uint32_t,char> jumps;
extern std::map<uint32_t,string> audio_address;
extern std::map<uint32_t,uint8_t> cached_jumps;
std::map<uint32_t, uint8_t*> jump_data;
extern std::map<uint32_t,uint32_t> rsp_reads;
extern std::map<uint32_t,uint32_t> rdram_reads;
std::map<uint32_t,bool> offsetHasAssembly;
void find_most_similar_function(uint32_t function_offset, string bytes);

string last_reversed_address = "";
bool should_reverse_jumps = false;
bool should_change_jumps = false;
int frame_last_reversed = 0;
int time_last_reversed = 0;
string game_name = "";
string ucode_crc = "";

// string next_dma_type = "";
// uint32_t previous_function = 0;
 std::vector<uint32_t> function_stack = std::vector<uint32_t>();
 std::vector<uint32_t> previous_ra;

extern std::map<string, string> function_signatures;

extern std::map<uint32_t, cdl_dram_cart_map> audio_samples;
extern std::map<uint32_t, cdl_dram_cart_map> cart_rom_dma_writes;
extern std::map<uint32_t, cdl_dram_cart_map> dma_sp_writes;
extern std::map<uint32_t, cdl_labels> labels;
extern std::map<uint32_t, cdl_jump_return> jump_returns;
extern std::map<uint32_t,cdl_tlb> tlbs;
extern std::map<uint32_t,cdl_dma> dmas;
std::map<uint32_t, std::map<string, string> > addresses;

uint32_t rspboot = 0;

#define NUMBER_OF_MANY_READS 40
#define NUMBER_OF_MANY_WRITES 40

// 
// # Toggles
// 
bool support_n64_prints = false;
bool cdl_log_memory = false;
bool tag_functions = false;
bool log_notes = false;
bool log_function_calls = false;
bool log_ostasks = false;
bool log_rsp = false;

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
        cdl_log_memory = !cdl_log_memory;
        tag_functions = !tag_functions;
        // should_change_jumps = true;
        //should_reverse_jumps = true;
        //time_last_reversed = time(0);
        // show_interface();
    }
}

bool createdCartBackup = false;
void backupCart() {
    game_name = alphabetic_only_name(ROM_PARAMS.headername, 21);
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
void saveLibUltraSignatures() {
    std::ofstream o("libultra.json");
    o << libultra_signatures.dump(1) << std::endl;
}

void setTogglesBasedOnConfig() {
    cdl_log_memory = reConfig["shouldLogMemory"];
    tag_functions = reConfig["shouldTagFunctions"];
    log_notes = reConfig["shouldLogNotes"];
    log_function_calls = reConfig["shouldLogFunctionCalls"];
    support_n64_prints = reConfig["shouldSupportN64Prints"];
    log_ostasks = reConfig["shouldLogOsTasks"];
    log_rsp = reConfig["shouldLogRsp"];
}

void readJsonFromFile() {
    readLibUltraSignatures();
    readJsonToObject("symbols.json", linker_map_file);
    readJsonToObject("./reconfig.json", reConfig);
    setTogglesBasedOnConfig();
    string filename = "./configs/";
    filename+=ROM_PARAMS.headername;
    filename += ".json";
    // read a JSON file
    if (!reConfig["startFreshEveryTime"]) {
        cout << "Reading previous game config file \n";
        readJsonToObject(filename, fileConfig);
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
    if (fileConfig.find("memory_to_log") == fileConfig.end())
                fileConfig["memory_to_log"] = R"([])"_json;

    memory_to_log = fileConfig["memory_to_log"].get< std::map<uint32_t,string> >();
    memory_to_log[0x0E5320] = "rsp.boot";
    jumps = fileConfig["jumps"].get< std::map<uint32_t,char> >();
    tlbs = fileConfig["tlbs"].get< std::map<uint32_t,cdl_tlb> >();
    dmas = fileConfig["dmas"].get< std::map<uint32_t,cdl_dma> >();
    rsp_reads = fileConfig["rsp_reads"].get< std::map<uint32_t,uint32_t> >();
    rdram_reads = fileConfig["rdram_reads"].get< std::map<uint32_t,uint32_t> >();
    labels = fileConfig["labels"].get< std::map<uint32_t,cdl_labels> >();
    jump_returns = fileConfig["jump_returns"].get< std::map<uint32_t,cdl_jump_return> >();
}

void saveJsonToFile() {
    string filename = "./configs/";
    filename += ROM_PARAMS.headername;
    filename += ".json";
    std::ofstream o(filename);
    o << fileConfig.dump(1) << std::endl;
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
    // only called in pure_interp mode
        // jump_data[program_counter] = op_address;
        // if (!labels[function_stack.back()].generatedSignature) {
        //     printf("Not generated sig yet: %#08x \n", *op_address);
        // }
    
}

int note_count = 0;
void add_note(uint32_t pc, uint32_t target, string problem) {
    if (!log_notes) return;
    if (labels[function_stack.back()].doNotLog) return;
    std::stringstream sstream;
    sstream << std::hex << "pc:0x" << pc << "-> 0x" << target;
    sstream << problem << " noteNumber:"<<note_count;
    // cout << sstream.str();
    labels[function_stack.back()].notes[pc] = sstream.str();
    note_count++;
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
    // uint8_t* header_bytes = (uint8_t*)&d.header;
    uint32_t header_bytes = __builtin_bswap32(d.header);
    std::stringstream sstream, header;
    string region_type = "bin";
    if (d.is_assembly) {
        region_type="asm";
    }
    string proper_cart_address_str = n2hexstr(d.rom_start);
    string ascii_header = d.ascii_header;
    header << " header: " <<  ascii_header << " 0x" << std::hex << header_bytes; // (header_bytes[3]+0) << (header_bytes[2]+0) << (header_bytes[1]+0) << (header_bytes[0]+0);
    sstream << "  - [0x" << std::hex << d.rom_start << ", 0x"<< (d.rom_start+d.length);
    sstream << ", \"" << region_type << "\",   ";
    if (d.known_name.length()>1) {
        sstream << "\"" << d.known_name;
    }
    else if (strcmp(d.guess_type.c_str(), "audio") == 0) {
        sstream << "\"" << d.guess_type << "_" << d.rom_start << "_len_"<< d.length;
    } else {
        sstream << "\"_" << ascii_header << "_" << "_" << d.rom_start << "_len_"<< d.length;
    }

    if (d.is_assembly) {
        sstream << "\", 0x" << n2hexstr(d.dram_start) << "] # frame:0x" << n2hexstr(d.frame);
    } else {
        sstream << "\"] # (DRAM:0x" << n2hexstr(d.dram_start) << ") (frame:0x" << n2hexstr(d.frame) << ") ";
    }

    if (strcmp(d.guess_type.c_str(), "audio") != 0) {
        sstream << header.str() << " trace:" << d.func_addr;
    }
    //sstream << " Func:" << d.func_addr;
    
    if (d.tbl_mapped_addr>0) {
        sstream << " Tbl mapped:"<<d.tbl_mapped_addr;
    }

    std::string mapping = sstream.str();
    return mapping;
}

uint32_t current_function = 0;
void log_dma_write(uint8_t* mem, uint32_t proper_cart_address, uint32_t cart_addr, uint32_t length, uint32_t dram_addr) {
    if (dmas.find(proper_cart_address) != dmas.end() ) 
        return;

    auto t = cdl_dma();
    t.dram_start=dram_addr;
    t.dram_end = dram_addr+length;
    t.rom_start = proper_cart_address;
    t.rom_end = proper_cart_address+length;
    t.length = length;
    t.ascii_header = get_header_ascii(mem, proper_cart_address);
    t.header = mem[proper_cart_address+3];
    t.frame = l_CurrentFrame;

    // if (function_stack.size() > 0 && labels.find(current_function) != labels.end()) {
    t.func_addr = print_function_stack_trace(); // labels[current_function].func_name;
    // }

    dmas[proper_cart_address] = t;

    // std::cout << "DMA: Dram:0x" << std::hex << t.dram_start << "->0x" << t.dram_end << " Length:0x" << t.length << " " << t.ascii_header << " Stack:" << function_stack.size() << " " << t.func_addr << " last:"<< function_stack.back() << "\n";
    
}

void cdl_finish_pi_dma(uint32_t a) {
    // cout <<std::hex<< "Finish PI DMA:" << a << "\n";
}
void cdl_finish_si_dma(uint32_t a) {
    cout <<std::hex<< "Finish SI DMA:" << a << "\n";
}
void cdl_finish_ai_dma(uint32_t a) {
    // cout <<std::hex<< "Finish AI DMA:" << (a & 0xffffff) << "\n";
}

void cdl_clear_dma_log() {
    // next_dma_type = "cleared";
}
void cdl_clear_dma_log2() {
    // next_dma_type = "interesting";
}

void cdl_log_cart_reg_access() {
    // next_dma_type = "bin";
    add_tag_to_function("_cartRegAccess", function_stack.back());
}

void cdl_log_dma_si_read() {
    add_tag_to_function("_dmaSiRead", function_stack.back());
}

void cdl_log_copy_pif_rdram() {
    add_tag_to_function("_copyPifRdram", function_stack.back());
}

void cdl_log_si_reg_access() {
    // COntrollers, rumble paks etc
    add_tag_to_function("_serialInterfaceRegAccess", function_stack.back());
}

void cdl_log_mi_reg_access() {
    // The MI performs the read, modify, and write operations for the individual pixels at either one pixel per clock or one pixel for every two clocks. The MI also has special modes for loading the TMEM, filling rectangles (fast clears), and copying multiple pixels from the TMEM into the framebuffer (sprites).
    add_tag_to_function("_miRegRead", function_stack.back());
}
void cdl_log_mi_reg_write() {
    // The MI performs the read, modify, and write operations for the individual pixels at either one pixel per clock or one pixel for every two clocks. The MI also has special modes for loading the TMEM, filling rectangles (fast clears), and copying multiple pixels from the TMEM into the framebuffer (sprites).
    add_tag_to_function("_miRegWrite", function_stack.back());
}

void cdl_log_pi_reg_read() {
    if (function_stack.size() > 0)
    add_tag_to_function("_piRegRead", function_stack.back());
}
void cdl_log_pi_reg_write() {
    if (function_stack.size() > 0)
    add_tag_to_function("_piRegWrite", function_stack.back());
}

void cdl_log_read_rsp_regs2() {
    add_tag_to_function("_rspReg2Read", function_stack.back());
}
void cdl_log_write_rsp_regs2() {
    add_tag_to_function("_rspReg2Write", function_stack.back());
}

void cdl_log_read_rsp_regs() {
    if (function_stack.size() > 0)
    add_tag_to_function("_rspRegRead", function_stack.back());
}
void cdl_log_write_rsp_regs() {
    if (function_stack.size() > 0)
    add_tag_to_function("_rspRegWrite", function_stack.back());
}

void cdl_log_update_sp_status() {
    if (function_stack.size() > 0)
    add_tag_to_function("_updatesSPStatus", function_stack.back());
}

void cdl_common_log_tag(const char* tag) {
    if (function_stack.size() > 0)
    add_tag_to_function(tag, function_stack.back());
}

void cdl_log_audio_reg_access() {
    // if (audio_functions.find(current_function) != audio_functions.end() ) 
    //     return;
    // if (function_stack.size() ==0 || labels.size() ==0 || function_stack.size() > 0xF) {
    //     return;
    // }
    // next_dma_type = "audio";
    // TODO speed this up with a check first
    add_tag_to_function("_audioRegAccess", function_stack.back());
    //cout << std::hex << labels[function_stack.back()].func_name << " function is audio\n";
}

string print_function_stack_trace() {
    if (function_stack.size() ==0 || labels.size() ==0 || function_stack.size() > 0xF) {
        return "";
    }
    std::stringstream sstream;
    for (auto& it : function_stack) {
        if (strcmp(labels[it].func_name.c_str(),"") == 0) {
            sstream << "0x" << std::hex << it<< "->";
            continue;
        }
        sstream << labels[it].func_name << "->";
    }
    // cout << "Stack:"<< sstream.str() << "\n";
    return sstream.str();
}




void resetReversing() {
    time_last_reversed = time(0);
    last_reversed_address="";
}

void save_cdl_files() {
    resetReversing();
    find_asm_sections();
    find_audio_sections();
    find_audio_functions();
    save_dram_rw_to_json();
    saveJsonToFile();
    saveLibUltraSignatures();
}

uint32_t cdl_get_alternative_jump(uint32_t current_jump) {
    if (!should_change_jumps) {
        return current_jump;
    }

    for (auto& it : linker_map_file.items()) {
        uint32_t new_jump = hex_to_int(it.key());
        cout << "it:" << it.value() << " = " << it.key() << " old:" << current_jump << " new:"<< new_jump << "\n";
        linker_map_file.erase(it.key());
        should_change_jumps = false;
        return new_jump;
    }

    return current_jump;
}


void write_rom_mapping() {
    save_cdl_files();
    printf("ROM_PARAMS.headername: %s \n", ROM_PARAMS.headername);
    string filename = "./configs/";
    filename+=ROM_PARAMS.headername;
    filename += ".config.yaml";
    ofstream file(filename, std::ios_base::binary);
    file << "# ROM splitter configuration file\n";
    file << "name: \"";
    file << ROM_SETTINGS.goodname;
    file << "\"\n";
    file << "# Graphics uCodeCRC: \"" << ucode_crc << "\"\n";
    file << "# checksums from ROM header offsets 0x10 and 0x14\n";
    file << "# used for auto configuration detection\n";
    file << "checksum1: 0x";
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
    file <<"  - [0x000B70, 0x001000, \"bin\",    \"bootcode_font\"]\n";    

    //
    // Write out 
    //
    for (auto& it : dmas) {
        auto t = it.second;
        if (it.first ==0 || t.dram_start == 0) continue;
        file << create_n64_split_regions(t) << "\n";
    }

    file <<"# Labels for functions or data memory addresses\n";
    file <<"# All label addresses are RAM addresses\n";
    file <<"# Order does not matter\n";
    file <<"labels:\n";
    uint32_t entryPoint = ROM_HEADER.PC; // (int8_t*)(void*)&ROM_HEADER.PC;
    file << "   - [0x" << std::hex << __builtin_bswap32(entryPoint)+0 <<", \"EntryPoint\"]\n";
    for (auto& it : labels) {
        auto t = it.second;
        if (strcmp(t.func_offset.c_str(), "") == 0) continue;
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

void cdl_log_jump_cached(int take_jump, uint32_t jump_target, uint8_t* jump_target_memory) {
    if (cached_jumps.find(jump_target) != cached_jumps.end() ) 
        return;
    cached_jumps[jump_target] = 1;
    cout << "Cached:" << std::hex << jump_target << "\n";
}

void log_function_call(uint32_t function_that_is_being_called) {
    if (!log_function_calls) return;
    uint32_t function_that_is_calling = function_stack.back();
    if (labels.find(function_that_is_calling) == labels.end()) return;
    if (labels[function_that_is_calling].isRenamed || labels[function_that_is_calling].doNotLog) return;
    labels[function_that_is_calling].function_calls[function_that_is_being_called] = labels[function_that_is_being_called].func_name;
}

const char *register_names[] = {
    "$r0",
    "$at",
    "v0", "v1",
    "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9",
    "k0", "k1",
    "$gp",
    "$sp",
    "sB",
    "$ra"
};
#define REGISTER_A0 4
#define REGISTER_A1 5
#define REGISTER_A2 6
#define REGISTER_A3 7
#define REGISTER_GP 28
#define REGISTER_SP 29
#define REGISTER_SB  30
#define REGISTER_RA  31

void cdl_log_jump_always(int take_jump, uint32_t jump_target, uint8_t* jump_target_memory, uint32_t ra, uint32_t pc) {
    add_note(ra-8, pc, "Call (jal)");
    previous_ra.push_back(ra);
    uint32_t previous_function_backup = function_stack.back();
    function_stack.push_back(jump_target);
    current_function = jump_target;

    if (jumps[jump_target] >3) return;
    jumps[jump_target] = 0x04;

    if (labels.find(jump_target) != labels.end() ) 
        return;
    log_function_call(jump_target);
    auto t = cdl_labels();
    string jump_target_str = n2hexstr(jump_target);
    t.func_offset = jump_target_str;
    if (labels.find(previous_function_backup) != labels.end()) {
        t.caller_offset = labels[previous_function_backup].func_name+" (ra:"+n2hexstr(ra)+")";
    } else {
        t.caller_offset = n2hexstr(previous_function_backup);
    }
    t.func_name = game_name+"_func_"+jump_target_str;
    t.func_stack = function_stack.size();
    t.stack_trace = print_function_stack_trace();
    t.doNotLog = false;
    t.many_memory_reads = false;
    t.many_memory_writes = false;
    labels[jump_target] = t;
    jump_data[jump_target] = jump_target_memory;
}
void cdl_log_jump_return(int take_jump, uint32_t jump_target, uint32_t pc, uint32_t ra, int64_t* registers, struct r4300_core* r4300) {
    uint32_t previous_function_backup = -1;

    // if (previous_function_backup > ra) {
    //     // cout << std::hex << " Odd the prev function start should never be before return address ra:" << ra << " previous_function_backup:" << previous_function_backup << "\n";
    //     return;
    // }

    if (function_stack.size()>0) {
        previous_function_backup = function_stack.back();
        
    }
    else {
        add_note(pc, jump_target, "function_stack <0");
        // probably jumping from exception?
        // cout << "Missed push back?" << std::hex << jump_target << " ra" << ra << " pc:"<< pc<< "\n";
        // return;
    }

    if (jump_target == previous_ra.back()) {
        add_note(pc, jump_target, "successful return");
        
    } else {
        string problem = "Expected $ra to be 0x";
        problem += n2hexstr(previous_ra.back());
        problem += " but was:";
        problem += n2hexstr(jump_target);
        add_note(pc, jump_target, problem);
        // return;
    }
    function_stack.pop_back();
    current_function = function_stack.back();
    previous_ra.pop_back();


if (support_n64_prints) {
    if (strcmp(labels[previous_function_backup].func_name.c_str(),"osSyncPrintf") ==0) {
        uint32_t* memory = fast_mem_access(r4300, registers[REGISTER_A2]);
        string swapped = string_endian_swap((const char*)memory);
        labels[function_stack.back()].printfs[swapped] = "";
        printf("\n%s > %s",labels[function_stack.back()].func_name.c_str(), swapped.c_str());
    }
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
    if (length<2) {
        return;
    }

    if (jump_data.find(previous_function_backup) != jump_data.end()) {
        uint64_t byte_len = length;
        if (byte_len > 0xFFFF) {
            byte_len = 0xFFFF;
        }
        // string bytes = printBytesToStr(jump_data[previous_function_backup], byte_len)+"_"+n2hexstr(length);
        string bytes_with_branch_delay = printBytesToStr(jump_data[previous_function_backup], byte_len+4)+"_"+n2hexstr(length+4);
        string word_pattern = printWordsToStr(jump_data[previous_function_backup], byte_len+4)+" L"+n2hexstr(length+4,4);
        labels[previous_function_backup].function_bytes = bytes_with_branch_delay;
        labels[previous_function_backup].doNotLog = true;
        labels[previous_function_backup].generatedSignature = true;
        // labels[previous_function_backup].function_bytes_endian = Swap4Bytes(bytes);
        
        // now check to see if its in the mario map
        // if (/*strcmp(game_name.c_str(),"SUPERMARIO") == 0 &&*/ linker_map_file.find( n2hexstr(previous_function_backup) ) != linker_map_file.end()) {
        //     string offset = n2hexstr(previous_function_backup);
        //     string func_name = linker_map_file[offset];
        //     cout << game_name << "It is in the map file!" << n2hexstr(previous_function_backup) << " as:" << linker_map_file[n2hexstr(previous_function_backup)] << "\n";
        //     function_signatures[bytes] = func_name;
        //     if (strcmp(func_name.c_str(),"gcc2_compiled.")==0) return; // we don't want gcc2_compiled labels
        //     libultra_signatures["function_signatures"][bytes_with_branch_delay] = func_name;
        //     labels[previous_function_backup].func_name = libultra_signatures["function_signatures"][bytes_with_branch_delay];
        //     return;
        // }
        

        // if it is a libultra function then lets name it
        if (libultra_signatures["library_signatures"].find(word_pattern) != libultra_signatures["library_signatures"].end()) {
            std::cout << "In library_signatures:" <<  word_pattern << " name:"<< libultra_signatures["library_signatures"][word_pattern] << "\n";
            labels[previous_function_backup].func_name = libultra_signatures["library_signatures"][word_pattern];
            labels[previous_function_backup].isRenamed = true;
            // return since we have already named this functions, don't need its signature to be saved
            return;
        }
        if (libultra_signatures["game_signatures"].find(word_pattern) != libultra_signatures["game_signatures"].end()) {
            // std::cout << "In game_signatures:" <<  word_pattern << " name:"<< libultra_signatures["game_signatures"][word_pattern] << "\n";
            labels[previous_function_backup].func_name = libultra_signatures["game_signatures"][word_pattern];
            // return since we have already named this functions, don't need its signature to be saved
            return;
        }
        // if it is a libultra function then lets name it
        if (libultra_signatures["function_signatures"].find(bytes_with_branch_delay) != libultra_signatures["function_signatures"].end()) {
            std::cout << "In old libultra:" <<  bytes_with_branch_delay << " name:"<< libultra_signatures["function_signatures"][bytes_with_branch_delay] << "\n";
            labels[previous_function_backup].func_name = libultra_signatures["function_signatures"][bytes_with_branch_delay];
            if (labels[previous_function_backup].func_name.find("_func_") != std::string::npos) {
                // this is a non renamed function as it was auto generated
                find_most_similar_function(previous_function_backup, word_pattern);
                libultra_signatures["game_signatures"][word_pattern] = labels[previous_function_backup].func_name;
            }
            else {
                libultra_signatures["library_signatures"][word_pattern] = labels[previous_function_backup].func_name;
                labels[previous_function_backup].isRenamed = true;
            }
            libultra_signatures["function_signatures"].erase(bytes_with_branch_delay);
            // return since we have already named this functions, don't need its signature to be saved
            return;
        }

        // if it is an OLD libultra function then lets name it (without branch delay)
        // if (libultra_signatures["function_signatures"].find(bytes) != libultra_signatures["function_signatures"].end()) {
        //     std::cout << "In OLDEST libultra:" <<  bytes << " name:"<< libultra_signatures["function_signatures"][bytes] << "\n";
        //     labels[previous_function_backup].func_name = libultra_signatures["function_signatures"][bytes];
        //     labels[previous_function_backup].isRenamed = true;
        //     libultra_signatures["function_signatures"][bytes_with_branch_delay] = labels[previous_function_backup].func_name;
        //     // delete the old non-branch delay version
        //     libultra_signatures["function_signatures"].erase(bytes);
        //     // return since we have already named this functions, don't need its signature to be saved
        //     return;
        // }

        find_most_similar_function(previous_function_backup, word_pattern);
        // cout << "word_pattern:" << word_pattern << "\n";

        if (function_signatures.find(word_pattern) == function_signatures.end()) {
            // save this new function to both libultra and trace json
            function_signatures[word_pattern] = labels[previous_function_backup].func_name;
            libultra_signatures["game_signatures"][word_pattern] = labels[previous_function_backup].func_name;
        } else {
            //function_signatures.erase(bytes);
            // function_signatures[word_pattern] = "Multiple functions";
            std::cout << "Multiple Functions for :" << *jump_data[previous_function_backup] << " len:" << length << " pc:0x"<< pc << " - 0x" << previous_function_backup << "\n";
        }
    }
}

void find_most_similar_function(uint32_t function_offset, string bytes) {
    string named_function_with_highest_distance = "";
    // string auto_generated_function_with_highest_distance = "";
    double highest_distance = 0;
    // double highest_auto_distance = 0;
    for(auto it = libultra_signatures["library_signatures"].begin(); it != libultra_signatures["library_signatures"].end(); ++it) {
        double distance = jaro_winkler_distance(bytes.c_str(), it.key().c_str());
        if (distance >= highest_distance) {
            string function_name = it.value();
            // if (!is_auto_generated_function_name(function_name)) {
                if (distance == highest_distance) {
                    named_function_with_highest_distance += "_or_";
                    named_function_with_highest_distance += function_name;
                } else {
                    highest_distance = distance;
                    named_function_with_highest_distance = function_name;
                }
            // } else {
            //     highest_auto_distance = distance;
            //     auto_generated_function_with_highest_distance=function_name;
            // }
        }
        // cout << "IT:" << it.value() << " distance:" << jaro_winkler_distance(bytes_with_branch_delay.c_str(), it.key().c_str()) << "\n";
    }
    uint32_t highest_distance_percent = highest_distance*100;
    // cout << "generated function_with_highest_distance to "<< std::hex << function_offset << " is:"<<auto_generated_function_with_highest_distance<<" with "<< std::dec << highest_auto_distance <<"%\n";
    if (highest_distance_percent>=95) {
        cout << "function will be renamed "<< std::hex << function_offset << " is:"<<named_function_with_highest_distance<<" with "<< std::dec << highest_distance_percent <<"%\n";
        labels[function_offset].func_name = named_function_with_highest_distance;
        labels[function_offset].isRenamed = true;
    } else if (highest_distance_percent>=90) {
        cout << "function_with_highest_distance to "<< std::hex << function_offset << " is:"<<named_function_with_highest_distance<<" with "<< std::dec << highest_distance_percent <<"%\n";
        labels[function_offset].func_name += "_predict_"+named_function_with_highest_distance+"_";
        labels[function_offset].func_name += (to_string(highest_distance_percent));
        labels[function_offset].func_name += "percent";
    }
}

// loop through and erse multiple functions
void erase_multiple_func_signatures() {
    // function_signatures
    // Multiple functions
}

bool is_auto_generated_function_name(string func_name) {
    if (func_name.find("_func_") != std::string::npos) {
                // this is a non renamed function as it was auto generated
                return true;
    }
    return false;
}

unsigned int find_first_non_executed_jump() {
    for(map<unsigned int, char>::iterator it = jumps.begin(); it != jumps.end(); ++it) {
        if ((it->second+0) <3) {
            return it->first;
        }
    }
    return -1;
}

int cdl_log_jump(int take_jump, uint32_t jump_target, uint8_t* jump_target_memory, uint32_t pc, uint32_t ra) {
    add_note(pc, jump_target, "jump");
    // if (previous_ra.size() > 0 && ra != previous_ra.back()) {
    //     cdl_log_jump_always(take_jump, jump_target, jump_target_memory, ra, pc);
    //     //previous_ra.push_back(ra);
    //     return take_jump;
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

void find_audio_sections() {
    printf("finding audio sections \n");
    for(map<u_int32_t, cdl_dma>::iterator it = dmas.begin(); it != dmas.end(); ++it) {
        u_int32_t address = it->second.dram_start;
        if (audio_address.find(address) == audio_address.end() ) 
            continue;
        dmas[address].guess_type = "audio";
        it->second.guess_type="audio";
    }
}


void add_tag_to_function(string tag, uint32_t labelAddr) {
    if (!tag_functions || labels[labelAddr].isRenamed) return;
    if (labels[labelAddr].func_name.find(tag) != std::string::npos) return;
    labels[labelAddr].func_name += tag;
}

void find_audio_functions() {
    printf("finding audio functions \n");
    for(map<u_int32_t, cdl_labels>::iterator it = labels.begin(); it != labels.end(); ++it) {
        cdl_labels label = it->second;
        if (label.isRenamed) {
            continue; // only do it for new functions
        }
        if (label.many_memory_reads) {
            add_tag_to_function("_manyMemoryReads", it->first);
        }
        if (label.many_memory_writes) {
            add_tag_to_function("_manyMemoryWrites", it->first);
        }
        for(map<string, string>::iterator it2 = label.read_addresses.begin(); it2 != label.read_addresses.end(); ++it2) {
            u_int32_t address = hex_to_int(it2->first);
            if (audio_address.find(address) != audio_address.end() ) 
            {
                cout << "Function IS audio:"<< label.func_name << "\n";
            }
            if (address>0x10000000 && address <= 0x107fffff) {
                cout << "Function accesses cart rom:"<< label.func_name << "\n";
            }
        }
        for(map<string, string>::iterator it2 = label.write_addresses.begin(); it2 != label.write_addresses.end(); ++it2) {
            u_int32_t address = hex_to_int(it2->first);
            if (audio_address.find(address) != audio_address.end() ) 
            {
                cout << "Function IS audio:"<< label.func_name << "\n";
            }
            if (address>0x10000000 && address <= 0x107fffff) {
                cout << "Function IS cart rom:"<< label.func_name << "\n";
            }
        }
    }
}
bool isAddressCartROM(u_int32_t address) {
    return (address>0x10000000 && address <= 0x107fffff);
}

void cdl_log_audio_sample(uint32_t saved_ai_dram, uint32_t saved_ai_length) {
    if (audio_samples.find(saved_ai_dram) != audio_samples.end() ) 
        return;
    auto t = cdl_dram_cart_map();
    t.dram_offset = n2hexstr(saved_ai_dram);
    t.rom_offset = n2hexstr(saved_ai_length);
    audio_samples[saved_ai_dram] = t;
    // printf("audio_plugin_push_samples AI_DRAM_ADDR_REG:%#08x length:%#08x\n", saved_ai_dram, saved_ai_length);
}

void cdl_log_cart_rom_dma_write(uint32_t dram_addr, uint32_t cart_addr, uint32_t length) {
    if (cart_rom_dma_writes.find(cart_addr) != cart_rom_dma_writes.end() ) 
        return;
    auto t = cdl_dram_cart_map();
    t.dram_offset = n2hexstr(dram_addr);
    t.rom_offset = n2hexstr(cart_addr);
    cart_rom_dma_writes[cart_addr] = t;
    printf("cart_rom_dma_write: dram_addr:%#008x cart_addr:%#008x length:%#008x\n", dram_addr, cart_addr, length);
}

void cdl_log_dma_sp_write(uint32_t spmemaddr, uint32_t dramaddr, uint32_t length, unsigned char *dram) {
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

inline void cdl_log_memory_common(const uint32_t lsaddr, uint32_t pc) {
    

    // if (addresses.find(lsaddr) != addresses.end() ) 
    //     return;
    // addresses[lsaddr] = currentMap;
}

void cdl_log_mem_read(const uint32_t lsaddr, uint32_t pc) {
    if (!cdl_log_memory) return;
    if (memory_to_log.find(lsaddr) != memory_to_log.end() ) 
    {
        cout << "Logging Mem Read for 0x"<< std::hex << lsaddr << " At PC:" << pc <<"\n";
    }

    if (labels[current_function].isRenamed || labels[current_function].doNotLog) {
        // only do it for new functions
        return;
    }

    if (labels[current_function].read_addresses.size() > NUMBER_OF_MANY_READS) {
        labels[current_function].many_memory_reads = true;
        return;
    }

    // auto currentMap = addresses[lsaddr];
    // currentMap[n2hexstr(lsaddr)]=labels[current_function].func_name+"("+n2hexstr(current_function)+"+"+n2hexstr(pc-current_function)+")";

    auto offset = pc-current_function;
    labels[current_function].read_addresses[n2hexstr(lsaddr)] = "func+0x"+n2hexstr(offset)+" pc=0x"+n2hexstr(pc);

    

}

void cdl_log_mem_write(const uint32_t lsaddr, uint32_t pc) {
    if (!cdl_log_memory) return;
    
    if (memory_to_log.find(lsaddr) != memory_to_log.end() ) 
    {
        cout << "Logging Mem Write to 0x"<< std::hex << lsaddr << " At PC:" << pc <<"\n";
    }

    if (labels[current_function].isRenamed || labels[current_function].doNotLog) {
        // only do it for new functions
        return;
    }

    if (labels[current_function].write_addresses.size() > NUMBER_OF_MANY_WRITES) {
        labels[current_function].many_memory_writes = true;
        return;
    }
    // auto currentMap = addresses[lsaddr];
    // currentMap[n2hexstr(lsaddr)]=labels[current_function].func_name+"("+n2hexstr(current_function)+"+"+n2hexstr(pc-current_function)+")";

    auto offset = pc-current_function;
    labels[current_function].write_addresses[n2hexstr(lsaddr)] = "+0x"+n2hexstr(offset)+" pc=0x"+n2hexstr(pc);

    
}

void cdl_hit_memory_log_point(uint32_t address) {
    if (address>0x10000000 && address <= 0x107fffff) {
        cout << "Cart Memory access!" << std::hex << address << " in:" << labels[current_function].func_name << "\n";
    }
}

void cdl_log_masked_write(uint32_t* address, uint32_t dst2) {
    if (!cdl_log_memory) return;
    // cout << "masked write:"<<std::hex<<dst<<" : "<<dst2<<"\n";
    // if (memory_to_log.find(address) != memory_to_log.end() ) 
    // {
    //     cout << "Logging Mem Write to 0x"<< std::hex << address << " At PC:" <<"\n";
    // }
}

void cdl_log_get_mem_handler(uint32_t address) {
    if (!cdl_log_memory) return;
    cdl_hit_memory_log_point(address);
    if (memory_to_log.find(address) != memory_to_log.end() ) 
    {
        cout << "Logging Mem cdl_log_get_mem_handler access 0x"<< std::hex << address <<"\n";
        cdl_hit_memory_log_point(address);
    }
}
void cdl_log_mem_read32(uint32_t address) {
    if (!cdl_log_memory) return;
    cdl_hit_memory_log_point(address);
    if (memory_to_log.find(address) != memory_to_log.end() ) 
    {
        cout << "Logging Mem cdl_log_mem_read32 access 0x"<< std::hex << address <<"\n";
        cdl_hit_memory_log_point(address);
    }
}
void cdl_log_mem_write32(uint32_t address) {
    if (!cdl_log_memory) return;
    cdl_hit_memory_log_point(address);
    if (memory_to_log.find(address) != memory_to_log.end() ) 
    {
        cout << "Logging Mem cdl_log_mem_write32 access 0x"<< std::hex << address <<"\n";
        cdl_hit_memory_log_point(address);
    }
}

string mapping_names[] = {
    "M64P_MEM_NOTHING",
    "M64P_MEM_NOTHING",
    "M64P_MEM_RDRAM",
    "M64P_MEM_RDRAMREG",
    "M64P_MEM_RSPMEM",
    "M64P_MEM_RSPREG",
    "M64P_MEM_RSP",
    "M64P_MEM_DP",
    "M64P_MEM_DPS",
    "M64P_MEM_VI",
    "M64P_MEM_AI",
    "M64P_MEM_PI",
    "M64P_MEM_RI",
    "M64P_MEM_SI",
    "M64P_MEM_FLASHRAMSTAT",
    "M64P_MEM_ROM",
    "M64P_MEM_PIF",
    "M64P_MEM_MI"
};

void cdl_log_memory_mappings(mem_mapping* mappings, uint32_t number_of_mappings) {
    for (int i=0; i<=number_of_mappings; i++) {
        std::stringstream s;
        auto mapping = mappings[i];
        s << "Mapping:" << mapping_names[mapping.type+0] << " " << std::hex << mapping.begin << "->" << mapping.end << "\n";
        fileConfig["memMap"][i] = s.str();
    }
}

#define OSTASK_GFX 1
#define OSTASK_AUDIO 2

void cdl_log_ostask(uint32_t type, uint32_t flags, uint32_t bootcode, uint32_t bootSize, uint32_t ucode, uint32_t ucodeSize, uint32_t ucodeData, uint32_t ucodeDataSize) {
    if (!log_ostasks) return;
    if (rspboot == 0) {
        rspboot = map_assembly_offset_to_rom_offset(bootcode,0);
        auto bootDma = cdl_dma();
        bootDma.dram_start=rspboot;
        bootDma.dram_end = rspboot+bootSize;
        bootDma.rom_start = rspboot;
        bootDma.rom_end = rspboot+bootSize;
        bootDma.length = bootSize;
        bootDma.frame = l_CurrentFrame;
        bootDma.func_addr = print_function_stack_trace(); 
        bootDma.known_name = "rsp.boot";
        dmas[rspboot] = bootDma;
    }
    uint32_t ucodeRom = map_assembly_offset_to_rom_offset(ucode,0);

    if (dmas.find(ucodeRom) != dmas.end() ) 
        return;
    printf("OSTask type:%#08x flags:%#08x bootcode:%#08x ucode:%#08x ucodeSize:%#08x ucodeData:%#08x ucodeDataSize:%#08x \n", type, flags, bootcode, ucode, ucodeSize, ucodeData, ucodeDataSize);
    uint32_t ucodeDataRom = map_assembly_offset_to_rom_offset(ucodeData,0);

    auto data = cdl_dma();
    data.dram_start=ucodeData;
    data.dram_end = ucodeData+ucodeDataSize;
    data.rom_start = ucodeDataRom;
    data.rom_end = ucodeDataRom+ucodeDataSize;
    data.length = ucodeDataSize;
    data.frame = l_CurrentFrame;
    data.func_addr = print_function_stack_trace(); 
    data.is_assembly = false;

    auto t = cdl_dma();
    t.dram_start=ucode;
    t.dram_end = ucode+ucodeSize;
    t.rom_start = ucodeRom;
    t.rom_end = ucodeRom+ucodeSize;
    t.length = ucodeSize;
    t.frame = l_CurrentFrame;
    t.func_addr = print_function_stack_trace(); 
    t.is_assembly = false;


    if (type == OSTASK_AUDIO) {
        t.guess_type = "rsp.audio";
        t.ascii_header = "rsp.audio";
        t.known_name = "rsp.audio";
        data.ascii_header = "rsp.audio.data";
        data.known_name = "rsp.audio.data";
    } else if (type == OSTASK_GFX) {
        t.guess_type = "rsp.graphics";
        t.ascii_header = "rsp.graphics";
        t.known_name = "rsp.graphics";
        data.ascii_header = "rsp.graphics.data";
        data.known_name = "rsp.graphics.data";
    } else {
        printf("other type:%#08x  ucode:%#08x \n",type, ucodeRom);
    }
    dmas[ucodeRom] = t;
    dmas[ucodeDataRom] = data;
}


#define CDL_ALIST 0
#define CDL_UCODE_CRC 2
void cdl_log_rsp(uint32_t log_type, uint32_t address, const char * extra_data) {
    if (!log_rsp) return;
    if (log_type == CDL_ALIST) {
        if (audio_address.find(address) != audio_address.end() ) 
            return;
        audio_address[address] = n2hexstr(address)+extra_data;
        // cout << "Alist address:" << std::hex << address << " " << extra_data << "\n";
        return;
    }
    if (log_type == CDL_UCODE_CRC) {
        ucode_crc = n2hexstr(address);
        return;
    }
    cout << "Log rsp\n";
}

void cdl_log_dpc_reg_write(uint32_t address, uint32_t value, uint32_t mask) {
    cdl_common_log_tag("writeDPCRegs");
}


} // end extern C