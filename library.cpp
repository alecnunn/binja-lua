#include "binaryninjaapi.h"
#include "luascriptingprovider.h"
#include <string>

using namespace BinaryNinja;

// Forward declarations for plugin commands
static void RunLuaScriptCommand(BinaryView* view, uint64_t addr, uint64_t length);

// Helper function to get Lua scripting provider
static Ref<ScriptingProvider> GetLuaProvider() {
    auto providers = ScriptingProvider::GetList();
    LogInfo("Found %zu scripting providers", providers.size());

    for (auto provider : providers) {
        LogInfo("Provider: %s", provider->GetName().c_str());
        if (provider->GetName() == "Lua") {
            LogInfo("Successfully found Lua scripting provider");
            return provider;
        }
    }
    LogError("Lua scripting provider not found in %zu available providers", providers.size());
    return nullptr;
}

// Plugin command implementations
static void RunLuaScriptCommand(BinaryView* view, uint64_t addr, uint64_t length) {
    auto provider = GetLuaProvider();
    if (!provider) {
        LogError("Lua scripting provider not available");
        return;
    }

    // Show file dialog to select Lua script
    std::string filename;
    if (!GetOpenFileNameInput(filename, "Select Lua Script", "*.lua")) {
        return; // User cancelled
    }

    // Read the script file
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        LogError("Failed to open Lua script: %s", filename.c_str());
        return;
    }

    // Read file contents
    fseek(file, 0, SEEK_END);
    long file_length = ftell(file);
    fseek(file, 0, SEEK_SET);

    std::string script_content;
    script_content.resize(file_length);
    fread(&script_content[0], 1, file_length, file);
    fclose(file);

    LogInfo("Executing Lua script: %s", filename.c_str());

    // Execute the script
    auto instance = provider->CreateNewInstance();
    if (instance) {
        instance->SetCurrentBinaryView(view);
        auto result = instance->ExecuteScriptInput(script_content);
        if (result != BNScriptingProviderExecuteResult::SuccessfulScriptExecution) {
            LogError("Script execution failed");
        }
    } else {
        LogError("Failed to create Lua scripting instance");
    }
}


extern "C"
{
    BN_DECLARE_CORE_ABI_VERSION

    BINARYNINJAPLUGIN bool CorePluginInit()
    {
        // Register the Lua scripting provider
        LuaScriptingProvider::RegisterLuaScriptingProvider();

        // Register Run Lua Script command
        PluginCommand::RegisterForRange("BinjaLua\\Run Lua Script...",
                                       "Execute a Lua script file in the current binary context",
                                       RunLuaScriptCommand,
                                       [](BinaryView* view, uint64_t addr, uint64_t length) { return true; });

        return true;
    }
}