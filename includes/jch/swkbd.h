#pragma once

#include <vpad/input.h>
#include <coreinit/filesystem.h>
#ifdef __cplusplus
extern "C" {
#endif

int swkbdInit(void);

void swkbdExit(void);

void swkbdShow(void);

int swkbdIsOpened(void);

char* swkbdProc(VPADStatus* vpad);

void swkbdDrawTV(void);

FSClient* swkbdGetFSClient(void);

extern char* swkbdGetTextBuffer(void);

void swkbdDrawDRC(void);

#ifdef __cplusplus
}
#endif
