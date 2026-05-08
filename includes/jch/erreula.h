/*
erreula.h,
authored by 
*/
#pragma once
#include <nn/erreula.h>
#include <vpad/input.h>

#ifdef __cplusplus
extern "C" {
#endif

int  erreula_init(FSClient* existingFSClient);
void erreula_exit(void);
void erreula_show(const char* title, const char* msg, const char* btn1, const char* btn2);
int  erreula_is_opened(void);
int  erreula_get_result(void);   // 1 = btn1/OK, 2 = btn2, 0 = not decided yet
void erreula_proc(VPADStatus* vpad);
void erreula_draw_tv(void);
void erreula_draw_drc(void);

#ifdef __cplusplus
}
#endif