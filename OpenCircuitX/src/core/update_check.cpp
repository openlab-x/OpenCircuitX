#include "update_check.h"
#include "core/version.h"
#include <wx/tokenzr.h>
#include <thread>
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")

const char* const OCX_LATEST_DOWNLOAD_URL =
    "https://github.com/openlab-x/OpenCircuitX/releases/latest/download/OpenCircuitX-Setup.exe";

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
