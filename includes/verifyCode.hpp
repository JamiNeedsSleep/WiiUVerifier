#pragma once
#ifndef VERIFYCODE
#define VERIFYCODE
#include <curl/curl.h>
#include <string>
#include "nlohmann/json.hpp"
#include <coreinit/filesystem.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include "jch/erreula.h"
#include "jmc/display.hpp"
using json = nlohmann::json;

struct WUVOutput {
    char salt[64];
    bool status;
};
static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    out->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool sendRequest(const char* userHash, WUVOutput *out) {
    
    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl = curl_easy_init();
    if (!curl) {
        ShowMessage("curl_easy_init failed!", true);
        curl_global_cleanup();
        return false;
    }

    // NotificationModule_AddInfoNotification("Starting request...");
    OSSleepTicks(OSSecondsToTicks(3));
    std::string response;
    std::string url = "https://wuv.melo.cafe/verifyUHASH";
    curl_easy_setopt(curl, CURLOPT_CAINFO, "/vol/content/rootca/isrg-root-x1-cross-signed.pem");
    curl_easy_setopt(curl, CURLOPT_PORT, 443L);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "JMC/WiiUVerifier/0.1.5");
    // HASH HEADERS!!
    std::string body = "{\"userHash\": \"" + std::string(userHash) + "\"}";
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // END HASH HEADERS!!

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_DNS_SERVERS, "8.8.8.8,8.8.4.4");
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_easy_cleanup(curl);
    curl_global_cleanup();


    if (res != CURLE_OK || httpCode != 200) {
        if (httpCode == 404 && !response.empty()) {
            json j = json::parse(response, nullptr, false);
            if (!j.is_discarded() && j.contains("error") &&
                j["error"].get<std::string>() == "Invalid Hash - Database!!") {

                erreula_show(
                    "JMC-001",
                    "The User Hash you entered was wrong.\nThis is the code you get from\n/cafe get-hash",
                    "Got it.",
                    nullptr
                );
                while (erreula_is_opened()) {
                    VPADStatus vpad;
                    VPADReadError err;
                    VPADRead(VPAD_CHAN_0, &vpad, 1, &err);

                    erreula_proc(&vpad);

                    BeginFrame();
                    nn::erreula::DrawTV();
                    nn::erreula::DrawDRC();
                    EndFrame();
                }
            }
        }
        return false;
    }

    try {
        json j = json::parse(response);
        out->status = true;
        std::string salt = j["salt"].get<std::string>();
        strncpy(out->salt, salt.c_str(), sizeof(out->salt) - 1);
        // strcat(out->salt, "-0.50m");
        return true;
    } catch (...) {
        ShowMessage("JSON parse failed!", true);
        return false;
    }
}
#endif