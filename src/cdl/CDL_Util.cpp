#include "CDL.hpp"

void to_json(json& j, const cdl_memory_map& p) {
        j = json{{"start", p.start}, 
        {"end", p.end},
        {"type", p.type}
        };
    }
void from_json(const json& j, cdl_memory_map& p) {
    j.at("start").get_to(p.start);
    j.at("end").get_to(p.end);
    j.at("type").get_to(p.type);
}

void to_json(json& j, const cdl_jump_return& p) {
        j = json{{"return_offset", p.return_offset}, 
        {"func_offset", p.func_offset},
        {"caller_offset", p.caller_offset}
        };
    }
void from_json(const json& j, cdl_jump_return& p) {
    j.at("return_offset").get_to(p.return_offset);
    j.at("func_offset").get_to(p.func_offset);
    j.at("caller_offset").get_to(p.caller_offset);
}

void to_json(json& j, const cdl_labels& p) {
    j = json{{"func_name", p.func_name}, 
        {"func_offset", p.func_offset},
        {"caller_offset", p.caller_offset},
        {"func_stack", p.func_stack},
        {"return_offset_from_start", p.return_offset_from_start},
        {"function_bytes", p.function_bytes},
        {"stack_trace", p.stack_trace},
        {"function_bytes_endian", p.function_bytes_endian},
        {"read_addresses", p.read_addresses},
        {"write_addresses", p.write_addresses},
        {"isRenamed", p.isRenamed},
        {"many_memory_reads", p.many_memory_reads},
        {"many_memory_writes", p.many_memory_writes},
        {"function_calls", p.function_calls},
        {"printfs", p.printfs},
        {"notes", p.notes},
    };
}
void from_json(const json& j, cdl_labels& p) {
    j.at("func_name").get_to(p.func_name);
    j.at("func_offset").get_to(p.func_offset);
    j.at("caller_offset").get_to(p.caller_offset);
    j.at("func_stack").get_to(p.func_stack);
    j.at("return_offset_from_start").get_to(p.return_offset_from_start);
    j.at("function_bytes").get_to(p.function_bytes);
    j.at("stack_trace").get_to(p.stack_trace);
    j.at("function_bytes_endian").get_to(p.function_bytes_endian);
    j.at("read_addresses").get_to(p.read_addresses);
    j.at("write_addresses").get_to(p.write_addresses);
    j.at("isRenamed").get_to(p.isRenamed);
    j.at("many_memory_reads").get_to(p.many_memory_reads);
    j.at("many_memory_writes").get_to(p.many_memory_writes);
    j.at("function_calls").get_to(p.function_calls);
    //j.at("notes").get_to(p.notes);
}

void to_json(json& j, const cdl_dram_cart_map& p) {
        j = json{{"dram_offset", p.dram_offset}, {"rom_offset", p.rom_offset}};
    }
void from_json(const json& j, cdl_dram_cart_map& p) {
    j.at("dram_offset").get_to(p.dram_offset);
    j.at("rom_offset").get_to(p.rom_offset);
}

void to_json(json& j, const cdl_tlb& p) {
        j = json{{"start", p.start}, {"end", p.end}, {"rom_offset", p.rom_offset}};
    }

void from_json(const json& j, cdl_tlb& p) {
    j.at("start").get_to(p.start);
    j.at("end").get_to(p.end);
    j.at("rom_offset").get_to(p.rom_offset);
}

uint32_t convert_endianness(uint32_t value) {
    return __builtin_bswap32(value);
}

void printf_endian_swap(const char* data) {
    printf("%s",string_endian_swap(data).c_str());
}

string string_endian_swap(const char* data) {
    std::stringstream ss;
    const uint32_t *i = (uint32_t*)data;
    while (i != NULL)
    {
        //const char*)convert_endianness(*i);
        const char* chars = (const char*)i;
        ss << chars[3] << chars[2] << chars[1] << chars[0];
        if (chars[0] == '\0' || chars[1] == '\0' || chars[2] == '\0' || chars[3] == '\0') break;
        i++;
    }
    return ss.str();

}

uint32_t hex_to_int(string str) {
    uint32_t x;   
    std::stringstream ss;
    ss << std::hex << str;
    ss >> x;
    return x;
}

void to_json(json& j, const cdl_dma& p) {
        j = json{{"dram_start", p.dram_start}, 
        {"dram_end", p.dram_end}, 
        {"rom_start", p.rom_start},
        {"rom_end", p.rom_end},
        {"length", p.length},
        {"header", p.header},
        {"frame", p.frame},
        {"tbl_mapped_addr", p.tbl_mapped_addr},
        {"is_assembly", p.is_assembly},
        {"ascii_header", p.ascii_header},
        {"func_addr", p.func_addr},
        {"guess_type", p.guess_type},
        {"known_name", p.known_name},
        };
    }

void from_json(const json& j, cdl_dma& p) {
    j.at("dram_start").get_to(p.dram_start);
    j.at("dram_end").get_to(p.dram_end);
    j.at("rom_start").get_to(p.rom_start);
    j.at("rom_end").get_to(p.rom_end);
    j.at("length").get_to(p.length);
    j.at("header").get_to(p.header);
    j.at("frame").get_to(p.frame);
    j.at("tbl_mapped_addr").get_to(p.tbl_mapped_addr);
    j.at("is_assembly").get_to(p.is_assembly);
    j.at("ascii_header").get_to(p.ascii_header);
    j.at("func_addr").get_to(p.func_addr);
    j.at("guess_type").get_to(p.guess_type);
    j.at("known_name").get_to(p.known_name);
}

string printBytesToStr(uint8_t* mem, uint32_t length=0x18) {
    std::stringstream sstream;
    //sstream << std::hex << std::setfill ('0') << std::setw(sizeof(T)*2);
    for (int i=0; i<=length; i++) {
        sstream << n2hexstr((uint8_t)mem[i], 2);
    }
    return sstream.str();
}
string printWordsToStr(uint8_t* mem, uint32_t length=0x18) {
    std::stringstream sstream;
    //sstream << std::hex << std::setfill ('0') << std::setw(sizeof(T)*2);
    for (int i=0; i<=length; i++) {
        sstream << n2hexstr((uint8_t)mem[i], 2);
        if ((i+1)%4 == 0) {
            sstream << " ";
        }
    }
    return sstream.str();
}

extern "C" {
void printBytes(uint8_t* mem, uint32_t cartAddr, uint32_t length=0x18) {
    for (int i=0; i<=length; i++) {
        printf(" %d:%#02x ", i, mem[cartAddr+i]);
        if ((i+1)%4 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}
void printWords(uint8_t* mem, uint32_t cartAddr, uint32_t length=0x18) {
    for (int i=0; i<=length; i+=1) {
        printf("%s", n2hexstr(mem[cartAddr+i]).c_str());
        if ((i+1)%4 == 0) {
            printf(" ");
        }
    }
    printf("\n");
}
}
string alphabetic_only_name(char* mem, int length) {
    std::stringstream sstream;
    for (int i=0; i<=length; i++) {
        char c = mem[i];
        if (!isalpha(c)) { continue; }
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