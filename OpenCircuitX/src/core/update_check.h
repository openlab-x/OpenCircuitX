#pragma once

#include <wx/wx.h>
#include <functional>

// Stable URL - always resolves to the newest GitHub release's installer,
// regardless of version. Requires every release to also upload a copy of
// the installer under this exact filename.
extern const char* const OCX_LATEST_DOWNLOAD_URL;

wxString OCXFetchRemoteVersion();
int      OCXCompareVersions(const wxString& local, const wxString& remote);

// Fetches the remote version on a background thread. If it's newer than
// OCX_VERSION_STRING, calls onNewer(remoteVersion) back on the GUI thread
// (via wxTheApp->CallAfter, so it stays valid even if a window closed while
// the fetch was in flight - guard any captured window with wxWeakRef).
// Fails silently if offline or unreachable. Safe to call once at startup.
void OCXCheckForUpdatesAsync(std::function<void(const wxString&)> onNewer);
