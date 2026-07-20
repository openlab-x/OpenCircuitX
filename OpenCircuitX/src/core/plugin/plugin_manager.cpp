#include "plugin_manager.h"
#include "ui/panels/output_panel.h"
#include <wx/dir.h>
#include <wx/filename.h>

//--
// Helpers that bridge the C-callback interface to the C++ OutputPanel
//--
// Static pointers reachable by C callbacks - safe because there is exactly one
// PluginManager instance per application.
static OutputPanel*              s_output    = nullptr;
static std::vector<PluginMenuItem>* s_menuItems = nullptr;

static void CB_LogMessage(const char* text)
{
    if (s_output) s_output->LogMessage(wxString::FromUTF8(text));
}

static void CB_LogError(const char* text)
{
    if (s_output) s_output->LogError(wxString::FromUTF8(text));
}

static void CB_AddToolsMenuItem(const char* label,
                                void (*callback)(void* userData),
                                void* userData)
{
    if (s_menuItems && label && callback)
        s_menuItems->push_back({ wxString::FromUTF8(label), callback, userData });
}

//--
// PluginManager
//--
PluginManager::PluginManager(OutputPanel* output)
    : m_output(output)
{
    s_output    = output;
    s_menuItems = &m_menuItems;

    m_ctx.LogMessage       = CB_LogMessage;
    m_ctx.LogError         = CB_LogError;
    m_ctx.AddToolsMenuItem = CB_AddToolsMenuItem;
}

PluginManager::~PluginManager()
{
    UnloadAll();
    s_output    = nullptr;
    s_menuItems = nullptr;
}

void PluginManager::LoadAll(const wxString& pluginDir)
{
    if (!wxDirExists(pluginDir))
        return;

    // Platform-appropriate extension
#if defined(__WXMSW__)
    const wxString ext = "dll";
#elif defined(__WXMAC__)
    const wxString ext = "dylib";
#else
    const wxString ext = "so";
#endif

    wxDir dir(pluginDir);
    if (!dir.IsOpened())
        return;

    wxString filename;
    bool ok = dir.GetFirst(&filename, "*." + ext, wxDIR_FILES);
    while (ok)
    {
        LoadOne(pluginDir + "/" + filename);
        ok = dir.GetNext(&filename);
    }

    if (m_plugins.empty())
        m_output->LogMessage("PluginManager: no plugins found in " + pluginDir);
    else
        m_output->LogMessage(wxString::Format("PluginManager: %d plugin(s) loaded.",
                                              (int)m_plugins.size()));
}

bool PluginManager::LoadOne(const wxString& path)
{
    LoadedPlugin p;
    p.path = path;
    p.lib  = std::make_unique<wxDynamicLibrary>();

    if (!p.lib->Load(path, wxDL_VERBATIM | wxDL_QUIET))
    {
        m_output->LogError("Plugin load failed: " + path);
        return false;
    }

    // Resolve required symbols
    auto nameFn     = (ocx_plugin_name_fn)    p.lib->GetSymbol("ocx_plugin_name");
    auto versionFn  = (ocx_plugin_version_fn) p.lib->GetSymbol("ocx_plugin_version");
    auto initFn     = (ocx_plugin_init_fn)    p.lib->GetSymbol("ocx_plugin_init");
    p.shutdown      = (ocx_plugin_shutdown_fn)p.lib->GetSymbol("ocx_plugin_shutdown");

    if (!nameFn || !versionFn || !initFn || !p.shutdown)
    {
        m_output->LogError("Plugin missing required exports: " + wxFileName(path).GetFullName());
        return false;
    }

    p.name    = wxString::FromUTF8(nameFn());
    p.version = wxString::FromUTF8(versionFn());

    if (!initFn(&m_ctx))
    {
        m_output->LogError("Plugin init() returned false: " + p.name);
        return false;
    }

    m_output->LogMessage("Plugin loaded: " + p.name + "  v" + p.version);
    m_plugins.push_back(std::move(p));
    return true;
}

void PluginManager::UnloadAll()
{
    // Shutdown in reverse load order
    for (int i = (int)m_plugins.size() - 1; i >= 0; --i)
    {
        if (m_plugins[i].shutdown)
            m_plugins[i].shutdown();
    }
    m_plugins.clear();
}
