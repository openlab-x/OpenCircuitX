#include "update_check.h"
#include "core/version.h"
#include <wx/tokenzr.h>
#include <thread>
#ifdef WIN32
#include <windows.h>
#include <wininet.h>
#else
#include <curl/curl.h>
#endif
#pragma comment(lib, "wininet.lib")

#ifndef WIN32
size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) 
{
    size_t total = size * nmemb;
    strlcat((char*)userdata, ptr, total);
    return total;
}
#endif

const char* const OCX_LATEST_DOWNLOAD_URL =
    "https://github.com/openlab-x/OpenCircuitX/releases/latest/download/OpenCircuitX-Setup.exe";

wxString OCXFetchRemoteVersion()
{   
    #ifdef WIN32
    HINTERNET hNet = InternetOpenA("OpenCircuitX/" OCX_VERSION_STRING,
                                   INTERNET_OPEN_TYPE_PRECONFIG,
                                   nullptr, nullptr, 0);
    if (!hNet) return wxEmptyString;

    HINTERNET hUrl = InternetOpenUrlA(hNet, OCX_UPDATE_URL, nullptr, 0,
                                      INTERNET_FLAG_SECURE |
                                      INTERNET_FLAG_RELOAD  |
                                      INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) { InternetCloseHandle(hNet); return wxEmptyString; }

    wxString result;
    char     buf[128];
    DWORD    bytesRead = 0;
    while (InternetReadFile(hUrl, buf, sizeof(buf) - 1, &bytesRead) && bytesRead > 0)
    {
        buf[bytesRead] = '\0';
        result += wxString::FromAscii(buf);
        bytesRead = 0;
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hNet);

    result.Trim(true).Trim(false);
    return result;
    #else

    wxString result;
    char buf[128] = {0};

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, OCX_UPDATE_URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    result = wxString::FromAscii(buf);

    result.Trim(true).Trim(false);
    return result;
    #endif
}

int OCXCompareVersions(const wxString& local, const wxString& remote)
{
    auto parse = [](const wxString& v, int& major, int& minor, int& patch) {
        wxStringTokenizer tok(v, ".");
        major = tok.HasMoreTokens() ? wxAtoi(tok.GetNextToken()) : 0;
        minor = tok.HasMoreTokens() ? wxAtoi(tok.GetNextToken()) : 0;
        patch = tok.HasMoreTokens() ? wxAtoi(tok.GetNextToken()) : 0;
    };

    int lMaj, lMin, lPat, rMaj, rMin, rPat;
    parse(local,  lMaj, lMin, lPat);
    parse(remote, rMaj, rMin, rPat);

    if (lMaj != rMaj) return lMaj < rMaj ? -1 : 1;
    if (lMin != rMin) return lMin < rMin ? -1 : 1;
    if (lPat != rPat) return lPat < rPat ? -1 : 1;
    return 0;
}

void OCXCheckForUpdatesAsync(std::function<void(const wxString&)> onNewer)
{
    std::thread([onNewer]() {
        wxString remote = OCXFetchRemoteVersion();
        if (remote.IsEmpty()) return;

        if (OCXCompareVersions(OCX_VERSION_STRING, remote) < 0)
            wxTheApp->CallAfter([onNewer, remote]() { onNewer(remote); });
    }).detach();
}
