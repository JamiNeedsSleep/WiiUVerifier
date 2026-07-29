#pragma once
#ifndef JMCHASHSYSTEM
#define JMCHASHSYSTEM
#include <iostream>
#include <cstring>
#include <string>
#include <coreinit/mcp.h>
#include <coreinit/systeminfo.h>
#include <coreinit/filesystem.h>
#include <coreinit/memdefaultheap.h>
#include <whb/sdcard.h>
#include <sys/stat.h>
#include <fstream>
#include "jch/swkbd.h"
#include "jmc/display.hpp"
struct JMCHashResult {
    char output[650];
    bool status;
};
/*bool writeFile(const char* path, const char* data) {
    FSClient* client = (FSClient*)MEMAllocFromDefaultHeap(sizeof(FSClient));
    if (!client) return false;
    FSAddClient(client, FS_ERROR_FLAG_NONE);

    FSCmdBlock cmd;
    FSInitCmdBlock(&cmd);

    FSFileHandle handle;
    FSStatus status = FSOpenFile(client, &cmd, path, "w", &handle, (FSErrorFlag)0);
    if (status != FS_STATUS_OK) {
        FSDelClient(client, FS_ERROR_FLAG_NONE);
        MEMFreeToDefaultHeap(client);
        return false;
    }

    FSWriteFile(client, &cmd, (uint8_t*)data, 1, strlen(data), handle, 0, (FSErrorFlag)0);
    FSCloseFile(client, &cmd, handle, (FSErrorFlag)0);
    FSDelClient(client, FS_ERROR_FLAG_NONE);
    MEMFreeToDefaultHeap(client);
    return true;
}*/
bool writeFile(const char* path, const char* data) {
    std::string fullpath = std::string("fs:") + path;
    std::ofstream file(fullpath);
    if (!file) return false;
    file << data;
    file.close();
    return true;
}
char ihatemyself[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
bool addToCompletedHash(JMCHashResult *inResult, JMCHashResult *outResult, char toaddon[]) {
    if (!inResult->output) {
        strcpy(outResult->output, toaddon);
        strcat(outResult->output, "$");
        return true;
    } else {
        strcat(outResult->output, toaddon);
        strcat(outResult->output, "$");
        return true;
    }
    return false;
}
bool hashValue(char value[], char hash[], char* salt) {
    if (!value || !hash) return false;
    size_t len = strlen(value);
    size_t len2 = strlen(salt);
    for (size_t i = 0; i < len; i++) { 
        const char* pos = strchr(ihatemyself, value[i]);
        if (pos) {
            int idx = (pos - ihatemyself + len2) % 62;
            hash[i] = ihatemyself[idx];
        } else {
            hash[i] = value[i];
        }
    }
    hash[len] = '\0';
    return true;
}
bool ProduceSystemHash(JMCHashResult *outResult, char* userhash, char* salt) {
    int mcp = MCP_Open();
    if (mcp < 0) { outResult->status = false; return false; }

    alignas(0x40) MCPSysProdSettings config {};
    if (MCP_GetSysProdSettings(mcp, &config)) { outResult->status = false; return false; }

    uint64_t osid = OSGetOSID();
    addToCompletedHash(outResult, outResult, salt);
    addToCompletedHash(outResult, outResult, config.serial_id);
    addToCompletedHash(outResult, outResult, config.model_number);
    addToCompletedHash(outResult, outResult, userhash);

    std::string titlestr = std::to_string(osid);
    addToCompletedHash(outResult, outResult, const_cast<char*>(titlestr.c_str()));
    char temp[650];
    hashValue(outResult->output, temp, salt);
    strcpy(outResult->output, temp);

    OSSleepTicks(OSSecondsToTicks(1));
    OSSleepTicks(OSSecondsToTicks(2));

    mkdir("fs:/vol/external01/WUV", 0777);

    // bool wrote = writeFile(path3, outResult->output);
    bool wrote = writeFile("/vol/external01/WUV/hash.wuhash", outResult->output);
    OSSleepTicks(OSSecondsToTicks(1));
    if (wrote) {
        ShowMessage("Wrote hash to SD CARD!!");
        BeginFrame();
        DrawMessage();
        EndFrame();
    } else {
    	ShowMessage("write failed!", true);
        BeginFrame();
        DrawMessage();
        EndFrame();
    }

    OSSleepTicks(OSSecondsToTicks(3));

    MCP_Close(mcp);
    return true;
}
#endif