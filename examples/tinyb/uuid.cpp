#include<string>
#include<string.h>
#include<cstdlib>
#include<iostream>
#include<stdio.h>
#include<iomanip>
#include<stdint.h>
#include <tinyb.hpp>

using namespace tinyb;

int main(int argc, char **argv) {


    std::string uuid_string(argv[1]);

    std::cout << uuid_string << std::endl;
/*
    uint64_t uuid[2];
    if (uuid_string.size() == 4 || uuid_string.size() == 8) {
    // 16bit UUID
       uuid[0] = strtoul(uuid_string.c_str(), NULL, 16) << 32 | 0x00001000UL; 
       uuid[1] = (0x80000080ULL << 32) | 0x5f9b34fbUL; 
    } else if (uuid_string.size() == 36) {
    // 128bit UUID
       char u[37];
       strcpy(u, uuid_string.c_str());

       if (u[9] == '-') {
            u[9] = ' ';
            uuid[0] = strtoul(u + 0, NULL, 16) << 32;
       } else
           return 1; 
       if (u[13] == '-') {
            u[13] = ' ';
            uuid[0] = uuid[0] | strtoul(u + 10, NULL, 16) << 16;
       } else
           return 1; 
       if (u[17] == '-') {
            u[17] = ' ';
            uuid[0] = uuid[0] | strtoul(u + 14, NULL, 16);
       } else
           return 1; 

       if (u[21] == '-') {
            u[21] = ' ';
            uuid[1] = strtoul(u + 18, NULL, 16) << 48;
       } else
           return 1; 
       uuid[1] = uuid[1] | strtoul(u + 22, NULL, 16);
    } else
        return 1;

    printf("%08lx-%04lx-%04lx-%04lx-%012lx\n",
        (uuid[0] >> 32),
        ((uuid[0] >> 16) & 0xFFFFULL),
        (uuid[0] & 0xFFFFULL),
        (uuid[1] >> 48),
        (uuid[1] & ~(0xFFFFULL << 48)));
*/
    BluetoothUUID uuid1(uuid_string);
    BluetoothUUID uuid2(argv[1]);

    std::cout << uuid1.get_string() << " " << uuid2.get_string() << std::endl; 

    return 0;
}
