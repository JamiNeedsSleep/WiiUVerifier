// START INCLUDES
#include <iostream>
#include <cstring>
#include <string>
#include <nn/ac.h>
#include <whb/proc.h>
#include <coreinit/systeminfo.h>
#include <sys/wait.h>
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
#include "jmc/display.hpp"
#include "verifyCode.hpp"
#include "jmc/hash.hpp"
#include "jch/swkbd.h"
#include "jch/erreula.h"
// END INCLUDES
/*
I AM AN IDIOT!!!
This debugging sucked so badly that it made me mentally insane for over 3 days,
*/
JMCHashResult resulte {};
WUVOutput outputbrr {};

int main(int argc, char **argv)
{
    WHBProcInit();
    FSInit();
    if (!WHBMountSdCard()) {
        WHBProcShutdown();
        return -1;
    }
    socket_lib_init();

    Display_Init();

    VPADInit();
    ACInitialize();
    ACConnect();
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
    ShowMessage(ipmsg);
    BeginFrame();
	DrawMessage();
	EndFrame();
    OSSleepTicks(OSSecondsToTicks(3));

    AXInit();
    ShowMessage("Welcome to the Wii U Verifier!");
    BeginFrame();
	DrawMessage();
	EndFrame();
    OSSleepTicks(OSSecondsToTicks(3));

    if (swkbdInit() != 0)
    {
        ShowMessage("swkbd init failed!", true);
        BeginFrame();
        DrawMessage();
        EndFrame();
        OSSleepTicks(OSSecondsToTicks(3));

        VPADShutdown();
        FSShutdown();

        AXQuit();
        Display_Shutdown();
        socket_lib_finish();
        WHBProcShutdown();
        return -1;
    }
    erreula_init(swkbdGetFSClient());
    swkbdShow();

    char* result = nullptr;

    bool done = false;
    while (!swkbdFinished())
    {
        VPADStatus vpad{};
        VPADRead(VPAD_CHAN_0, &vpad, 1, nullptr);

        VPADGetTPCalibratedPoint(
            VPAD_CHAN_0,
            &vpad.tpNormal,
            &vpad.tpNormal
        );

        swkbdProc(&vpad);

        BeginFrame();
        swkbdDrawTV();
        swkbdDrawDRC();
        EndFrame();
    }

    result = swkbdGetTextBuffer();
    if (result)
    {
        ShowMessage("Sending verification...");
        BeginFrame();
        DrawMessage();
        EndFrame();
        bool success = sendRequest(result, &outputbrr);

        if (success) {
            ShowMessage("Success!");
            BeginFrame();
        	DrawMessage();
        	EndFrame();
        } else {
            ShowMessage("Verification failed!", true);
            BeginFrame();
        	DrawMessage();
        	EndFrame();
            goto exit;
        }

        OSSleepTicks(OSSecondsToTicks(1));
    }
    ProduceSystemHash(&resulte, swkbdGetTextBuffer(), outputbrr.salt);
    ShowMessage(resulte.output);
    BeginFrame();
    DrawMessage();
    EndFrame();
    swkbdExit();

    SDL_Reset();
    while (WHBProcIsRunning()) {
        SDL_Event ev;
        bool wantExit = false;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_CONTROLLERBUTTONDOWN &&
                ev.cbutton.button == SDL_CONTROLLER_BUTTON_GUIDE) {
                wantExit = true;
            }
        }
        if (wantExit) break;

        BeginFrame();
        EndFrame();
    }
exit:
    OSShutdown();
    Display_Shutdown();
    // Correct shutdown order:
    OSSleepTicks(OSSecondsToTicks(1));
    AXQuit();
    VPADShutdown();
    FSShutdown();
    socket_lib_finish();
    WHBProcShutdown();
    return 0;
}
