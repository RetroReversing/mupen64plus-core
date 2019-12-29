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
unsigned int hex_to_int(string str);
extern "C" {
void printBytes(uint8_t* mem, uint32_t cartAddr, uint32_t length);
void printWords(uint8_t* mem, uint32_t cartAddr, uint32_t length);
double jaro_winkler_distance(const char *s, const char *a);
}
void printf_endian_swap(const char* data);
string string_endian_swap(const char* data);
string alphabetic_only_name(char* mem, int length);
string get_header_ascii(uint8_t* mem, uint32_t proper_cart_address);
string printBytesToStr(uint8_t* mem, uint32_t length) ;
string printWordsToStr(uint8_t* mem, uint32_t length);
void save_dram_rw_to_json();
void readJsonToObject(string filename, json& json_object);

#define Swap4Bytes(val) \
 ( (((val) >> 24) & 0x000000FF) | (((val) >>  8) & 0x0000FF00) | \
(((val) <<  8) & 0x00FF0000) | (((val) << 24) & 0xFF000000) )


struct cdl_tlb {
    uint32_t start;
    uint32_t end;
    uint32_t rom_offset;
};
struct cdl_memory_map {
    uint32_t start;
    uint32_t end;
    string type;
};
struct cdl_dma {
    uint32_t dram_start;
    uint32_t dram_end;
    uint32_t rom_start;
    uint32_t rom_end;
    uint32_t length;
    uint32_t header;
    uint32_t frame;
    uint32_t tbl_mapped_addr;
    string func_addr;
    bool is_assembly;
    string ascii_header;
    string guess_type;
    string known_name;
};
struct cdl_dram_cart_map {
    string dram_offset;
    string rom_offset;
};
struct cdl_labels {
    string func_offset;
    string caller_offset;
    string func_name;
    string stack_trace;
    uint32_t func_stack;
    uint32_t return_offset_from_start;
    string function_bytes;
    string function_bytes_endian;
    std::map<uint32_t, string> function_calls;
    std::map<string, string> read_addresses;
    std::map<string, string> write_addresses;
    std::map<string, string> printfs;
    std::map<int32_t, string> notes;
    bool isRenamed;
    bool doNotLog;
    bool many_memory_writes;
    bool many_memory_reads;
    bool generatedSignature;
};
struct cdl_jump_return {
    string func_offset;
    string return_offset;
    string caller_offset;
};

void to_json(json& j, const cdl_jump_return& p);
void from_json(const json& j, cdl_jump_return& p);

void to_json(json& j, const cdl_labels& p);
void from_json(const json& j, cdl_labels& p);

void to_json(json& j, const cdl_dram_cart_map& p);
void from_json(const json& j, cdl_dram_cart_map& p);

void to_json(json& j, const cdl_dma& p);
void from_json(const json& j, cdl_dma& p);

void to_json(json& j, const cdl_tlb& p);
void from_json(const json& j, cdl_tlb& p);

template <typename I> std::string n2hexstr(I w, size_t hex_len = sizeof(I)<<1) {
    static const char* digits = "0123456789ABCDEF";
    std::string rc(hex_len,'0');
    for (size_t i=0, j=(hex_len-1)*4 ; i<hex_len; ++i,j-=4)
        rc[i] = digits[(w>>j) & 0x0f];
    return rc;
}

string to_hex(int my_integer);