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
json reConfig;

std::map<uint32_t,uint32_t> rsp_reads;
std::map<uint32_t,uint32_t> rdram_reads;
std::map<uint32_t,cdl_tlb> tlbs;
std::map<uint32_t,cdl_dma> dmas = std::map<uint32_t,cdl_dma>();
std::map<uint32_t,char> jumps;
std::map<uint32_t,string> audio_address;
std::map<uint32_t, cdl_dram_cart_map> audio_samples;
std::map<uint32_t, cdl_dram_cart_map> cart_rom_dma_writes;
std::map<uint32_t, cdl_dram_cart_map> dma_sp_writes;
std::map<uint32_t, cdl_labels> labels;
std::map<uint32_t, cdl_jump_return> jump_returns;
std::map<string, string> function_signatures;
std::map<uint32_t,uint8_t> cached_jumps;
std::map<uint32_t,string> memory_to_log;

void readJsonToObject(string filename, json& json_object) {
    std::ifstream i(filename);
    if (i.good()) {
        i >> json_object;
    }
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
    fileConfig["cached_jumps"] = cached_jumps;
    fileConfig["memory_to_log"] = memory_to_log;
    fileConfig["audio_address"] = audio_address;
    // fileConfig["addresses"] = addresses;
}