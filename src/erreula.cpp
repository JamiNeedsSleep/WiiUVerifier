// jch/erreula.cpp
#include "jch/erreula.h"
#include <coreinit/filesystem.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/thread.h>
#include <nn/erreula.h>
#include <stdlib.h>

extern "C" {

static nn::erreula::CreateArg s_createArg;
static FSClient*               s_fsClient = nullptr;
static int                     s_result   = 0;  // 1 = left/btn1, 2 = right/btn2

int erreula_init(FSClient* existingFSClient)
{
    s_fsClient = (FSClient *)MEMAllocFromDefaultHeap(sizeof(FSClient));
    if (!s_fsClient) return -3;

    FSAddClient(s_fsClient, FS_ERROR_FLAG_NONE);

    s_createArg.workMemory = MEMAllocFromDefaultHeap(nn::erreula::GetWorkMemorySize());
    if (!s_createArg.workMemory) return -2;

    s_createArg.region   = nn::erreula::RegionType::Europe;
    s_createArg.language = nn::erreula::LangType::English;
    s_createArg.fsClient = s_fsClient;

    if (!nn::erreula::Create(s_createArg)) return -1;

    return 0;
}

void erreula_exit(void)
{
    nn::erreula::Destroy();
    OSSleepTicks(OSSecondsToTicks(1));

    MEMFreeToDefaultHeap(s_createArg.workMemory);
    s_createArg.workMemory = nullptr;

    if (s_fsClient) {  // only free if we allocated it
        FSDelClient(s_fsClient, FS_ERROR_FLAG_NONE);
        MEMFreeToDefaultHeap(s_fsClient);
    }
    s_fsClient = nullptr;
}

// title/msg/btn are ASCII; btn2 may be nullptr for a single-button dialog
void erreula_show(const char* title, const char* msg, const char* btn1, const char* btn2)
{
    s_result = 0;

    static char16_t titleBuf[128], msgBuf[512], btn1Buf[64], btn2Buf[64];

    auto toU16 = [](const char* src, char16_t* dst, size_t max) {
        size_t i = 0;
        while (src && src[i] && i < max - 1) { dst[i] = (char16_t)(unsigned char)src[i]; i++; }
        dst[i] = u'\0';
    };

    toU16(title, titleBuf, 128);
    toU16(msg,   msgBuf,   512);
    toU16(btn1,  btn1Buf,  64);

    nn::erreula::AppearArg arg;
    // AppearArg wraps ErrorArg as errorArg
    arg.errorArg.errorTitle   = titleBuf;
    arg.errorArg.errorMessage = msgBuf;
    arg.errorArg.button1Label = btn1Buf;

    if (btn2) {
        toU16(btn2, btn2Buf, 64);
        arg.errorArg.button2Label = btn2Buf;
        arg.errorArg.errorType    = nn::erreula::ErrorType::Message2Button;
    } else {
        arg.errorArg.button2Label = nullptr;
        arg.errorArg.errorType    = nn::erreula::ErrorType::Message1Button;
    }

    nn::erreula::AppearErrorViewer(arg);
}

int erreula_is_opened(void)
{
    return nn::erreula::GetStateErrorViewer() != nn::erreula::State::Hidden;
}

int erreula_get_result(void)
{
    return s_result;
}

void erreula_proc(VPADStatus* vpad)
{
    nn::erreula::ControllerInfo info;
    info.vpad    = vpad;
    info.kpad[0] = nullptr;
    info.kpad[1] = nullptr;
    info.kpad[2] = nullptr;
    info.kpad[3] = nullptr;

    nn::erreula::Calc(info);

    if (nn::erreula::IsDecideSelectButtonError()) {
        // IsDecideSelectLeftButtonError / Right tell us which was pressed
        if (nn::erreula::IsDecideSelectLeftButtonError()) {
            s_result = 1;  // btn1 (left)
        } else if (nn::erreula::IsDecideSelectRightButtonError()) {
            s_result = 2;  // btn2 (right)
        } else {
            s_result = 1;  // fallback
        }
        nn::erreula::DisappearErrorViewer();
    }
}

void erreula_draw_tv(void)
{
    if (erreula_is_opened())
        nn::erreula::DrawTV();
}

void erreula_draw_drc(void)
{
    if (erreula_is_opened())
        nn::erreula::DrawDRC();
}

} // extern "C"