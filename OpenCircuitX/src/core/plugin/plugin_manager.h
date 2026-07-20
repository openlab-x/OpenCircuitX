#pragma once
#include "plugin_api.h"
#include <wx/dynlib.h>
#include <memory>
#include <vector>

//--
// PluginMenuItem - collected from all plugins, wired into the Tools menu
//--
struct PluginMenuItem
{
    wxString  label;
    void    (*callback)(void* userData);
    void*     userData;
};

//--
// LoadedPlugin - one entry per successfully loaded plugin
//--
struct LoadedPlugin
{
    wxString                          path;
    wxString                          name;
    wxString                          version;
    std::unique_ptr<wxDynamicLibrary> lib;   // unique_ptr keeps LoadedPlugin move-only
    ocx_plugin_shutdown_fn            shutdown = nullptr;
};

//--
// PluginManager
//
// Usage in MainWindow constructor (after output panel is ready):
//
//   pluginManager = new PluginManager(outputPanel);
//   pluginManager->LoadAll(executableDir + "/plugins");
//
//--
class OutputPanel;   // forward decl - avoids header dependency

class PluginManager
{
public:
    explicit PluginManager(OutputPanel* output);
    ~PluginManager();

    // Scan directory for .dll/.so/.dylib files and load each one.
    void LoadAll(const wxString& pluginDir);

    // Unload all plugins - called automatically in destructor.
    void UnloadAll();

    int  GetCount() const { return (int)m_plugins.size(); }

    // Menu items registered by plugins via OCXPluginContext::AddToolsMenuItem.
    const std::vector<PluginMenuItem>& GetMenuItems() const { return m_menuItems; }

private:
    bool LoadOne(const wxString& path);

    OutputPanel*              m_output;
    std::vector<LoadedPlugin>  m_plugins;
    std::vector<PluginMenuItem> m_menuItems;  // collected from all plugins
    OCXPluginContext           m_ctx;
};
