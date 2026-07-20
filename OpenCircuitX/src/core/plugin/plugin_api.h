#pragma once
//--
// OpenCircuitX Plugin API - v1
//
// A plugin is a shared library (.dll / .so / .dylib) that exports the four
// C-linkage symbols below.  OpenCircuitX discovers and loads plugins at
// startup from the "plugins/" folder next to the executable.
//
// Minimum plugin skeleton:
//
//   #include "plugin_api.h"
//   extern "C" {
//       const char* ocx_plugin_name()    { return "My Plugin"; }
//       const char* ocx_plugin_version() { return "1.0.0"; }
//       bool ocx_plugin_init(OCXPluginContext* ctx)
//       {
//           ctx->LogMessage("My Plugin loaded.");
//           return true;
//       }
//       void ocx_plugin_shutdown() {}
//   }
//--
#include <wx/wx.h>

//--
// OCXPluginContext - passed to ocx_plugin_init().
// Provides the services a plugin can call into.
//--
struct OCXPluginContext
{
    // Append a line to the Output tab.
    void (*LogMessage)(const char* text)  = nullptr;

    // Append a line to the Error tab (shown in red).
    void (*LogError)(const char* text)    = nullptr;

    // Add a menu item to the Tools menu.
    // When the user clicks it, callback(userData) is called on the main thread.
    void (*AddToolsMenuItem)(const char* label,
                             void (*callback)(void* userData),
                             void* userData)                  = nullptr;

    // Reserved for future expansion - always zero-initialised.
    void* reserved[8] = {};
};

//--
// Symbols that every plugin DLL must export (extern "C")
//--
// Human-readable plugin name (e.g. "My Synthesis Plugin")
typedef const char* (*ocx_plugin_name_fn)();

// SemVer string (e.g. "1.2.0")
typedef const char* (*ocx_plugin_version_fn)();

// Called once after load.  Return false to abort - plugin will be unloaded.
typedef bool (*ocx_plugin_init_fn)(OCXPluginContext* ctx);

// Called before the plugin is unloaded (app exit or future hot-unload).
typedef void (*ocx_plugin_shutdown_fn)();
