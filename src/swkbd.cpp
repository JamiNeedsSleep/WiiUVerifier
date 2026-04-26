#include "jch/swkbd.h"
#include <coreinit/filesystem.h>
#include <coreinit/memdefaultheap.h>
#include <stdlib.h>
#include <coreinit/thread.h>
#include <nn/swkbd.h>

extern "C" {

static nn::swkbd::CreateArg createArg;
static FSClient* fsClient;
static char textBuffer[0x200];
static nn::swkbd::AppearArg appearArg;
int swkbdInit(void)
{
    fsClient = (FSClient *)MEMAllocFromDefaultHeap(sizeof(FSClient));
    if (!fsClient) return -3;  // Add this!
    FSAddClient(fsClient, FS_ERROR_FLAG_NONE);
    // Create swkbd
    createArg.regionType = nn::swkbd::RegionType::Europe;
    createArg.workMemory = MEMAllocFromDefaultHeap(nn::swkbd::GetWorkMemorySize(1));
    if (!createArg.workMemory)
        return -2;
    createArg.fsClient = fsClient;
    if (!nn::swkbd::Create(createArg)) {
        return -1;
    }
    nn::swkbd::MuteAllSound(false);
    return 0;
}

void swkbdExit(void)
{
   nn::swkbd::Destroy();
   OSSleepTicks(OSSecondsToTicks(1));
   MEMFreeToDefaultHeap(createArg.workMemory);

   FSDelClient(fsClient, FS_ERROR_FLAG_NONE);
   MEMFreeToDefaultHeap(fsClient);
}

void swkbdShow(void)
{
    appearArg.keyboardArg.configArg.languageType = nn::swkbd::LanguageType::English;
    appearArg.keyboardArg.configArg.disableNewLine = true;
    appearArg.inputFormArg.hintText = u"User Hash!";
    appearArg.inputFormArg.maxTextLength = 20;
    appearArg.inputFormArg.type = nn::swkbd::InputFormType::InputForm0;
    nn::swkbd::AppearInputForm(appearArg);
}

int swkbdIsOpened(void)
{
    return nn::swkbd::GetStateInputForm() != nn::swkbd::State::Hidden;
}
FSClient* swkbdGetFSClient(void) {
    return fsClient;
}
char* swkbdGetTextBuffer(void) {
    return textBuffer;
}
char* swkbdProc(VPADStatus* vpad)
{
    nn::swkbd::ControllerInfo controllerInfo;
    controllerInfo.vpad = vpad;
    controllerInfo.kpad[0] = nullptr;
    controllerInfo.kpad[1] = nullptr;
    controllerInfo.kpad[2] = nullptr;
    controllerInfo.kpad[3] = nullptr;
    nn::swkbd::Calc(controllerInfo);

    if (nn::swkbd::IsNeedCalcSubThreadFont()) {
        nn::swkbd::CalcSubThreadFont();
    }

    if (nn::swkbd::IsNeedCalcSubThreadPredict()) {
        nn::swkbd::CalcSubThreadPredict();
    }

    if (nn::swkbd::IsDecideOkButton(nullptr)) {
        nn::swkbd::DisappearInputForm();
        
        // Copy string immediately before it's gone
        const char16_t* str = nn::swkbd::GetInputFormString();
        if (str) {
            for (size_t i = 0; i < sizeof(textBuffer); i++) {
                if (!str[i]) { textBuffer[i] = '\0'; break; }
                textBuffer[i] = str[i] > 0x7F ? '?' : (char)str[i];
            }
        }
        // DON'T return yet - wait for swkbd to fully close
        // Return nullptr so the loop keeps running and calling Calc
        // until swkbdIsOpened() returns false
        return nullptr; // ← keep returning null until hidden
    }

    if (nn::swkbd::IsDecideCancelButton(nullptr)) {
        nn::swkbd::DisappearInputForm();
    }

    return nullptr;
}

void swkbdDrawTV(void)
{
    if (swkbdIsOpened()) {
        nn::swkbd::DrawTV();
    }
}

void swkbdDrawDRC(void)
{
    if (swkbdIsOpened()) {
        nn::swkbd::DrawDRC();
    }
}

}
