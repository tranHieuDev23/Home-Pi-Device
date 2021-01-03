#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include <vector>
#include <string>
#include <WiFi.h>

class WifiScanResult
{
public:
    const String ssid;
    const bool open;

    WifiScanResult(const String &ssid, const bool &open) : ssid(ssid), open(open) {}
};

std::vector<WifiScanResult> scanWiFiNetworks()
{
    int n = WiFi.scanNetworks();
    auto results = std::vector<WifiScanResult>();
    results.reserve(n);
    for (int i = 0; i < n; i++)
    {
        results.push_back(WifiScanResult(WiFi.SSID(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN));
    }
    return results;
}

#endif