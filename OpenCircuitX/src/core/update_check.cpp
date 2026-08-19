#include "update_check.h"
#include "core/version.h"
#include <wx/tokenzr.h>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#else
#include <curl/curl.h>
#include <algorithm>
#include <cstring>
#endif

const char* const OCX_LATEST_DOWNLOAD_URL =
    "https://github.com/openlab-x/OpenCircuitX/releases/latest/download/OpenCircuitX-Setup.exe";

#ifdef _WIN32

wxString OCXFetchRemoteVersion()
{
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
}

#else

namespace {

// Bounded write target for OCXCurlWriteCallback - callback enforces
// `capacity` itself since curl chunk sizes are attacker/server controlled
// and must never be trusted as the destination buffer's size.
struct FetchBuffer
{
    char*  data;
    size_t capacity;  // usable bytes in `data`, not counting the null terminator
    size_t length;    // bytes written so far
};

size_t OCXCurlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    FetchBuffer* buf   = static_cast<FetchBuffer*>(userdata);
    size_t       total = size * nmemb;
    size_t       room  = (buf->capacity > buf->length) ? (buf->capacity - buf->length) : 0;
    size_t       toCopy = std::min(total, room);

    if (toCopy > 0)
    {
        std::memcpy(buf->data + buf->length, ptr, toCopy);
        buf->length += toCopy;
    }
    buf->data[buf->length] = '\0';

    // Report the full chunk as consumed even if truncated, otherwise curl
    // treats a short return as a write error and aborts the transfer.
    return total;
}

} // namespace

wxString OCXFetchRemoteVersion()
{
    CURL* curl = curl_easy_init();
    if (!curl) return wxEmptyString;

    char        buf[128];
    FetchBuffer fetchBuf{ buf, sizeof(buf) - 1, 0 };
    buf[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, OCX_UPDATE_URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OCXCurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fetchBuf);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "OpenCircuitX/" OCX_VERSION_STRING);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) return wxEmptyString;

    wxString result = wxString::FromAscii(buf);
    result.Trim(true).Trim(false);
    return result;
}

#endif

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
