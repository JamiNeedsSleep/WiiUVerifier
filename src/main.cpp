// START INCLUDES
#include <iostream>
#include <cstring>
#include <string>
#include <nn/ac.h>
#include <whb/proc.h>
#include <whb/log_console.h>
#include <coreinit/systeminfo.h>
#include <sys/wait.h>
#include <whb/log.h>
#include <whb/gfx.h>
#include <whb/sdcard.h>
#include <whb/libmanager.h>
#include <nsysnet/_socket.h>
#include <sndcore2/core.h>
#include <coreinit/filesystem.h>
#include <coreinit/thread.h>
#include <coreinit/launch.h>
#include <sysapp/launch.h>
#include <vpad/input.h>
#include <coreinit/time.h>
#include <notifications/notifications.h>
#include "verifyCode.hpp"
#include "jmc/hash.hpp"
#include "jch/swkbd.h"
// END INCLUDES
/*
I AM AN IDIOT!!!
This debugging sucked so badly that it made me mentally insane for over 3 days,
*/
JMCHashResult resulte {};
int main(int argc, char **argv)
{
    WHBProcInit();
    FSInit();
    if (!WHBMountSdCard()) {

        WHBProcShutdown();
        return -1;
    }
    socket_lib_init();
    VPADInit();
    ACInitialize();
    ACConnect();
    NotificationModule_InitLibrary();
    OSSleepTicks(OSSecondsToTicks(3));
    OSEnableHomeButtonMenu(false);
    // Check if we actually have an IP
    unsigned int ip = 0;
    ACGetAssignedAddress(&ip);
    char ipmsg[64];
    snprintf(ipmsg, sizeof(ipmsg), "IP: %d.%d.%d.%d",
        (ip >> 24) & 0xFF,
        (ip >> 16) & 0xFF,
        (ip >>  8) & 0xFF,
        (ip >>  0) & 0xFF);
    NotificationModule_AddInfoNotification(ipmsg);
    OSSleepTicks(OSSecondsToTicks(3));
    AXInit();
    WHBLogConsoleInit();
    WHBGfxInit();
    WHBLogConsoleSetColor(4251856);
    NotificationModule_AddInfoNotification("Welcome to the Wii U Verifier!");
    OSSleepTicks(OSSecondsToTicks(3));
    if (swkbdInit() != 0)
    {
        NotificationModule_AddErrorNotification("swkbd init failed!");
        WHBLogConsoleDraw();

        WHBLogConsoleFree();

        VPADShutdown();
        FSShutdown();

        AXQuit();
        NotificationModule_DeInitLibrary();
        WHBGfxShutdown();
        socket_lib_finish();
        WHBProcShutdown();
        return -1;
    }

    swkbdShow();

    char* result = nullptr;

    bool done = false;
    while (WHBProcIsRunning() && !done)
    {
        VPADStatus vpad;
        VPADRead(VPAD_CHAN_0, &vpad, 1, nullptr);
        VPADGetTPCalibratedPoint(VPAD_CHAN_0, &vpad.tpNormal, &vpad.tpNormal);
        result = swkbdProc(&vpad);

        WHBGfxBeginRender();

        WHBGfxBeginRenderTV();
        WHBGfxClearColor(0.0f, 0.0f, 1.0f, 1.0f);
            // DON'T draw log while swkbd is open
        swkbdDrawTV();
        WHBGfxFinishRenderTV();

        WHBGfxBeginRenderDRC();
        WHBGfxClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        swkbdDrawDRC();
        WHBGfxFinishRenderDRC();

        WHBGfxFinishRender();

        // Only break when swkbd is fully closed AND we have a result
        if (strlen(swkbdGetTextBuffer()) > 0 && !swkbdIsOpened())
            done = true;
    }
    result = swkbdGetTextBuffer();
    if (result)
    {
        NotificationModule_AddInfoNotification("Sending verification...");
        bool success = sendRequest(result);

        if (success) {
            NotificationModule_AddInfoNotification("Success!");
        } else {
            NotificationModule_AddErrorNotification("Verification failed!");
            goto exit;
        }

        OSSleepTicks(OSSecondsToTicks(1));
    }
    ProduceSystemHash(&resulte, swkbdGetTextBuffer());
    NotificationModule_AddInfoNotification(resulte.output);
    swkbdExit();
    while (WHBProcIsRunning()) {
        VPADStatus vpad2;
        VPADRead(VPAD_CHAN_0, &vpad2, 1, nullptr);
        VPADGetTPCalibratedPoint(VPAD_CHAN_0, &vpad2.tpNormal, &vpad2.tpNormal);
        if (vpad2.trigger & VPAD_BUTTON_HOME) {
            goto exit;
        }

        WHBGfxBeginRender();
        WHBGfxBeginRenderTV();
        WHBGfxClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        WHBGfxFinishRenderTV();
        WHBGfxBeginRenderDRC();
        WHBGfxClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        WHBGfxFinishRenderDRC();
        WHBGfxFinishRender();
    }
exit:
    OSShutdown();
    WHBGfxShutdown();
    // Correct shutdown order:
    OSSleepTicks(OSSecondsToTicks(1));
    WHBLogConsoleFree();
    AXQuit();
    VPADShutdown();
    NotificationModule_DeInitLibrary();
    FSShutdown();
    socket_lib_finish();
    WHBProcShutdown();   // ← Always last
    return 0;
}

/*
OLD CODE (REFERENCE ONLY!):
int main(int argc, char **argv)
{
    WHBProcInit();
    WHBLogConsoleInit();
    int mcp = MCP_Open();
    if (mcp < 0) {
        WHBLogPrint("ERROR!!!");
        return 1;
    }
    char stringbean[] = "Hash: ";
    char filler[] = "-*";
    uint64_t titleid;
    char hashbeforehash[36];
    WHBLogConsoleSetColor(4251856);
    uint64_t osid = OSGetOSID();
    WHBLogPrint("Welcome to the Wii U Verifier!");
    if (MCP_GetTitleId(mcp, &titleid)) {
        WHBLogPrint("Whoops! MCP ERROR!!");
    } else {
        std::string titlestr = std::to_string(titleid);
        const char* c_titlestr_data = titlestr.c_str();
        char titlechar_array[30];
        strcpy(titlechar_array, c_titlestr_data);
        std::string osidstr = std::to_string(osid);
        const char* c_osidstr_data = osidstr.c_str();
        char osidchar_array[40];
        strcpy(osidchar_array, c_osidstr_data);
        strcpy(hashbeforehash, titlechar_array);
        strcat(hashbeforehash, filler);
        strcat(hashbeforehash, osidchar_array);
        strcat(hashbeforehash, filler);
        WHBLogPrint("====================================================");
    }
    alignas(0x40) MCPSysProdSettings config {};
    if (MCP_GetSysProdSettings(mcp, &config)) {
        WHBLogPrint("Whoops! MCP ERROR!!");
    } else {
        strcat(hashbeforehash, config.serial_id);
        char wthman[150];
        strcpy(wthman, stringbean);
        strcat(wthman, hashbeforehash);
        WHBLogPrint(wthman);
    }

    MCP_Close(mcp);
    while (WHBProcIsRunning()) {
        WHBLogConsoleDraw();
    }

    WHBLogConsoleFree();
    WHBProcShutdown();

    return 0;
} */