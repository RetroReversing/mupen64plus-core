#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

extern "C" {
// TODO: need to log input and then call input.keyDown(keymod, keysym);

void main_state_save(int format, const char *filename);
void main_state_load(const char *filename);
void show_interface();
void corruptBytes(uint8_t* mem, uint32_t cartAddr, int times);

extern int   l_CurrentFrame;

// 
// # Variables
// 
int corrupt_start =  0xb2b77c;
int corrupt_end = 0xb2b77c;
int difference = corrupt_end-corrupt_start;



void cdl_keyevents(int keysym, int keymod) {
    printf("event_sdl_keydown frame:%d key:%d modifier:%d \n", l_CurrentFrame, keysym, keymod);
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
        show_interface();
    }
}

void printBytes(uint8_t* mem, uint32_t cartAddr) {
    for (int i=0; i<=0x18; i++) {
        printf(" %d:%#02x ", i, mem[cartAddr+i]);
        if ((i+1)%4 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

bool createdCartBackup = false;
void backupCart() {
    std::cout << "TODO: backup";
    createdCartBackup = true;
}
void resetCart() {
    std::cout << "TODO: reset";
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
    if (proper_cart_address >= corrupt_start && proper_cart_address <= corrupt_end) { //l_CurrentFrame == 0x478 && length == 0x04) { //} proper_cart_address == 0xb4015c) {
        printf("save_state_before\n");
        main_state_save(0, "before_corruption");
        printBytes(mem, proper_cart_address);
        printf("MODIFIED IT!! %#08x\n\n\n", mem[proper_cart_address+1]);
        corruptBytes(mem, proper_cart_address, 10);
        printBytes(mem, proper_cart_address);
    }
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


} // end extern C