#pragma once

#include "binaryninjaapi.h"
#include <lua.hpp>
#include <memory>
#include <unordered_map>
#include <string>

using namespace BinaryNinja;

class LuaScriptingProvider : public ScriptingProvider
{
public:
    LuaScriptingProvider();
    virtual ~LuaScriptingProvider();

    virtual Ref<ScriptingInstance> CreateNewInstance() override;
    virtual bool LoadModule(const std::string& repository, const std::string& module, bool force) override;
    virtual bool InstallModules(const std::string& modules) override;

    static void RegisterLuaScriptingProvider();
};

class LuaScriptingInstance : public ScriptingInstance
{
private:
    lua_State* m_luaState;
    std::unordered_map<std::string, std::string> m_loadedModules;
    Ref<BinaryView> m_currentBinaryView;
    Ref<Function> m_currentFunction;
    Ref<BasicBlock> m_currentBasicBlock;
    uint64_t m_currentAddress;
    uint64_t m_selectionBegin;
    uint64_t m_selectionEnd;
    BNScriptingProviderInputReadyState m_inputReadyState;
    Ref<Logger> m_logger;

    // Cache for magic variable context to avoid unnecessary updates
    Ref<BinaryView> m_lastCachedBinaryView;
    Ref<Function> m_lastCachedFunction;
    Ref<BasicBlock> m_lastCachedBasicBlock;
    uint64_t m_lastCachedAddress;
    uint64_t m_lastCachedSelectionBegin;
    uint64_t m_lastCachedSelectionEnd;

    void InitializeLuaState();
    void SetupBindings();
    void UpdateMagicVariables();
    void UpdateMagicVariableValues();
    void SetupUtilityFunctions();
    bool HasContextChanged() const;
    void UpdateContextCache();
    void CleanupLuaState();

    std::string SafeToString(lua_State* L, int index);

    static int LuaPrint(lua_State* L);
    static int LuaError(lua_State* L);
    static int LuaWarn(lua_State* L);

public:
    LuaScriptingInstance(ScriptingProvider* provider);
    virtual ~LuaScriptingInstance();

    virtual BNScriptingProviderExecuteResult ExecuteScriptInput(const std::string& input) override;
    virtual BNScriptingProviderExecuteResult ExecuteScriptInputFromFilename(const std::string& filename) override;
    virtual void CancelScriptInput() override;
    virtual void SetCurrentBinaryView(BinaryView* view) override;
    virtual void SetCurrentFunction(Function* func) override;
    virtual void SetCurrentBasicBlock(BasicBlock* block) override;
    virtual void SetCurrentAddress(uint64_t addr) override;
    virtual void SetCurrentSelection(uint64_t begin, uint64_t end) override;
    virtual std::string CompleteInput(const std::string& text, uint64_t state) override;
    virtual void Stop() override;

    lua_State* GetLuaState() const { return m_luaState; }
    Ref<Logger> GetLogger() const { return m_logger; }
};
