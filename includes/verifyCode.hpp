#pragma once
#ifndef VERIFYCODE
#define VERIFYCODE
#include <curl/curl.h>
#include <string>
#include "nlohmann/json.hpp"
#include <whb/proc.h>
#include <whb/log_console.h>
#include <coreinit/filesystem.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>

#include <notifications/notifications.h>
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
        NotificationModule_AddErrorNotification("curl_easy_init failed!");
        curl_global_cleanup();
        return false;
    }

    // NotificationModule_AddInfoNotification("Starting request...");
    OSSleepTicks(OSSecondsToTicks(3));
    std::string response;
    std::string url = "https://wuv.greemdev.net/verifyUHASH";
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
        NotificationModule_AddErrorNotification("An error occurred!");
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
        NotificationModule_AddErrorNotification("JSON parse failed!");
        return false;
    }
}
#endif