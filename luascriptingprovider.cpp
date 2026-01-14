#include "luascriptingprovider.h"
#include <lauxlib.h>
#include <lualib.h>

#include "bindings/common.h"

static const char* LUA_USERDATA_METATABLE = "BinaryNinja";

LuaScriptingProvider::LuaScriptingProvider() : ScriptingProvider("Lua", "lua5.4")
{
}

LuaScriptingProvider::~LuaScriptingProvider()
{
}

Ref<ScriptingInstance> LuaScriptingProvider::CreateNewInstance()
{
    return new LuaScriptingInstance(this);
}

bool LuaScriptingProvider::LoadModule(const std::string& repository, const std::string& module, bool force)
{
    return false;
}

bool LuaScriptingProvider::InstallModules(const std::string& modules)
{
    return false;
}

void LuaScriptingProvider::RegisterLuaScriptingProvider()
{
    static LuaScriptingProvider* provider = new LuaScriptingProvider();
    ScriptingProvider::Register(provider);
}

LuaScriptingInstance::LuaScriptingInstance(ScriptingProvider* provider)
    : ScriptingInstance(provider), m_luaState(nullptr), m_currentAddress(0),
      m_selectionBegin(0), m_selectionEnd(0), m_inputReadyState(ReadyForScriptProgramInput),
      m_lastCachedAddress(0), m_lastCachedSelectionBegin(0), m_lastCachedSelectionEnd(0)
{
    // Create logger for this plugin instance
    m_logger = LogRegistry::CreateLogger("BinjaLua");
    InitializeLuaState();
}

LuaScriptingInstance::~LuaScriptingInstance()
{
    CleanupLuaState();
}

void LuaScriptingInstance::InitializeLuaState()
{
    try {
        m_luaState = luaL_newstate();
        if (!m_luaState) {
            m_logger->LogError("Failed to create Lua state");
            Error("Failed to create Lua state");
            return;
        }

        luaL_openlibs(m_luaState);

        lua_pushlightuserdata(m_luaState, this);
        lua_setglobal(m_luaState, "__binja_instance");

        lua_pushcfunction(m_luaState, LuaPrint);
        lua_setglobal(m_luaState, "print");

        lua_pushcfunction(m_luaState, LuaError);
        lua_setglobal(m_luaState, "error");

        lua_pushcfunction(m_luaState, LuaWarn);
        lua_setglobal(m_luaState, "warn");

        SetupBindings();

        // Notify that we're ready for input
        InputReadyStateChanged(m_inputReadyState);
    } catch (const std::exception& e) {
        m_logger->LogError("InitializeLuaState failed: %s", e.what());
        Error("Failed to initialize Lua state: " + std::string(e.what()));
    } catch (...) {
        m_logger->LogError("InitializeLuaState failed: Unknown error occurred");
        Error("Failed to initialize Lua state: Unknown error");
    }
}

void LuaScriptingInstance::SetupBindings()
{
    if (!m_luaState) return;

    try {
        m_logger->LogDebug("SetupBindings - starting");
        BinjaLua::RegisterAllBindings(m_luaState, m_logger);
        m_logger->LogDebug("SetupBindings - bindings registered");

        // Setup utility functions (dump, get_selected_data, etc.) once during init
        SetupUtilityFunctions();

        UpdateMagicVariables();
        m_logger->LogDebug("SetupBindings - completed");
    } catch (const std::exception& e) {
        m_logger->LogError("SetupBindings failed: %s", e.what());
    } catch (...) {
        m_logger->LogError("SetupBindings failed: Unknown error occurred");
    }
}

void LuaScriptingInstance::UpdateMagicVariables()
{
    if (!m_luaState) return;
    
    // Only update magic variables if the context has actually changed
    // This avoids expensive re-computation on every script execution
    if (HasContextChanged()) {
        m_logger->LogDebug("Context changed, updating magic variables");
        UpdateMagicVariableValues();
        UpdateContextCache();
    } else {
        m_logger->LogDebug("Context unchanged, skipping magic variable update");
    }
}

void LuaScriptingInstance::CleanupLuaState()
{
    if (m_luaState) {
        lua_close(m_luaState);
        m_luaState = nullptr;
    }
}

int LuaScriptingInstance::LuaPrint(lua_State* L)
{
    lua_getglobal(L, "__binja_instance");
    LuaScriptingInstance* instance = static_cast<LuaScriptingInstance*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    if (!instance) return 0;

    int n = lua_gettop(L);
    std::string result;
    
    for (int i = 1; i <= n; i++) {
        if (i > 1) result += "\t";
        
        size_t len;
        const char* str = luaL_tolstring(L, i, &len);
        if (str) {
            result += std::string(str, len);
            lua_pop(L, 1);
        }
    }
    
    instance->Output(result + "\n");
    instance->m_logger->LogInfo("Script output: %s", result.c_str());
    return 0;
}

int LuaScriptingInstance::LuaError(lua_State* L)
{
    lua_getglobal(L, "__binja_instance");
    LuaScriptingInstance* instance = static_cast<LuaScriptingInstance*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    if (!instance) return 0;

    size_t len;
    const char* msg = luaL_checklstring(L, 1, &len);
    std::string message(msg, len);
    instance->Error(message + "\n");
    instance->m_logger->LogError("Script error: %s", msg);
    return 0;
}

int LuaScriptingInstance::LuaWarn(lua_State* L)
{
    lua_getglobal(L, "__binja_instance");
    LuaScriptingInstance* instance = static_cast<LuaScriptingInstance*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    if (!instance) return 0;

    size_t len;
    const char* msg = luaL_checklstring(L, 1, &len);
    std::string message(msg, len);
    instance->Warning(message + "\n");
    instance->m_logger->LogWarn("Script warning: %s", msg);
    return 0;
}

BNScriptingProviderExecuteResult LuaScriptingInstance::ExecuteScriptInput(const std::string& input)
{
    if (!m_luaState) {
        m_logger->LogError("ExecuteScriptInput called with uninitialized Lua state");
        Error("Lua state not initialized");
        return InvalidScriptInput;
    }

    m_logger->LogDebug("ExecuteScriptInput: %s", input.c_str());

    // Set state to executing
    m_inputReadyState = NotReadyForInput;
    InputReadyStateChanged(m_inputReadyState);

    UpdateMagicVariables();

    BNScriptingProviderExecuteResult scriptResult;

    // First try to execute as an expression by wrapping with "return"
    std::string expressionCode = "return " + input;
    int result = luaL_dostring(m_luaState, expressionCode.c_str());

    if (result == LUA_OK) {
        // Expression executed successfully - check if we have a return value to print
        if (lua_gettop(m_luaState) > 0 && !lua_isnil(m_luaState, -1)) {
            // For tables, use dump() for pretty printing
            if (lua_istable(m_luaState, -1)) {
                lua_getglobal(m_luaState, "dump");
                if (lua_isfunction(m_luaState, -1)) {
                    lua_pushvalue(m_luaState, -2);  // Push the table
                    if (lua_pcall(m_luaState, 1, 1, 0) == LUA_OK) {
                        size_t len;
                        const char* str = lua_tolstring(m_luaState, -1, &len);
                        if (str) {
                            Output(std::string(str, len) + "\n");
                        }
                        lua_pop(m_luaState, 1);  // Pop dump result
                    } else {
                        lua_pop(m_luaState, 1);  // Pop error
                    }
                } else {
                    lua_pop(m_luaState, 1);  // Pop non-function dump
                    // Fallback to standard display
                    Output("<table>\n");
                }
            } else {
                // Use luaL_tolstring for non-tables (handles __tostring metamethods)
                size_t len;
                const char* str = luaL_tolstring(m_luaState, -1, &len);
                if (str) {
                    Output(std::string(str, len) + "\n");
                    lua_pop(m_luaState, 1);  // Pop the string created by luaL_tolstring
                }
            }
        }
        lua_settop(m_luaState, 0); // Clear stack
        scriptResult = SuccessfulScriptExecution;
    } else {
        // Expression failed, clear error and try as statement
        lua_pop(m_luaState, 1);

        result = luaL_dostring(m_luaState, input.c_str());

        if (result == LUA_OK) {
            scriptResult = SuccessfulScriptExecution;
        } else {
            // Handle error safely
            std::string error_msg = SafeToString(m_luaState, -1);
            lua_pop(m_luaState, 1); // Clean up error
            if (!error_msg.empty()) {
                Error(error_msg + "\n");
            }
            scriptResult = InvalidScriptInput;
        }
    }
    
    // Reset to ready state
    m_inputReadyState = ReadyForScriptProgramInput;
    InputReadyStateChanged(m_inputReadyState);
    
    return scriptResult;
}

BNScriptingProviderExecuteResult LuaScriptingInstance::ExecuteScriptInputFromFilename(const std::string& filename)
{
    if (!m_luaState) {
        m_logger->LogError("ExecuteScriptInputFromFilename called with uninitialized Lua state");
        Error("Lua state not initialized");
        return InvalidScriptInput;
    }

    m_logger->LogDebug("ExecuteScriptInputFromFilename: %s", filename.c_str());

    // Set state to executing
    m_inputReadyState = NotReadyForInput;
    InputReadyStateChanged(m_inputReadyState);

    UpdateMagicVariables();

    int result = luaL_dofile(m_luaState, filename.c_str());
    
    // Reset to ready state
    m_inputReadyState = ReadyForScriptProgramInput;
    InputReadyStateChanged(m_inputReadyState);
    
    if (result != LUA_OK) {
        size_t len;
        const char* error_msg = lua_tolstring(m_luaState, -1, &len);
        if (error_msg) {
            std::string errorStr(error_msg, len);
            m_logger->LogError("Script execution failed in file %s: %s", filename.c_str(), errorStr.c_str());
            Error(errorStr);
        } else {
            m_logger->LogError("Script execution failed in file %s: Unknown error", filename.c_str());
            Error("Script execution failed: Unknown error");
        }
        lua_pop(m_luaState, 1);
        return InvalidScriptInput;
    }

    return SuccessfulScriptExecution;
}

void LuaScriptingInstance::CancelScriptInput()
{
}

void LuaScriptingInstance::SetCurrentBinaryView(BinaryView* view)
{
    m_currentBinaryView = view;
}

void LuaScriptingInstance::SetCurrentFunction(Function* func)
{
    m_currentFunction = func;
}

void LuaScriptingInstance::SetCurrentBasicBlock(BasicBlock* block)
{
    m_currentBasicBlock = block;
}

void LuaScriptingInstance::SetCurrentAddress(uint64_t addr)
{
    m_currentAddress = addr;
}

void LuaScriptingInstance::SetCurrentSelection(uint64_t begin, uint64_t end)
{
    m_selectionBegin = begin;
    m_selectionEnd = end;
}

std::string LuaScriptingInstance::CompleteInput(const std::string& text, uint64_t state)
{
    return "";
}

void LuaScriptingInstance::Stop()
{
}


std::string LuaScriptingInstance::SafeToString(lua_State* L, int index)
{
    if (!L || !lua_checkstack(L, 2)) {
        return "<stack error>";
    }
    
    int type = lua_type(L, index);
    
    try {
        switch (type) {
            case LUA_TSTRING: {
                size_t len;
                const char* str = lua_tolstring(L, index, &len);
                return str ? std::string(str, len) : "<invalid string>";
            }
            case LUA_TNUMBER: {
                if (lua_isinteger(L, index)) {
                    return std::to_string(lua_tointeger(L, index));
                } else {
                    return std::to_string(lua_tonumber(L, index));
                }
            }
            case LUA_TBOOLEAN: {
                return lua_toboolean(L, index) ? "true" : "false";
            }
            case LUA_TNIL: {
                return "nil";
            }
            case LUA_TUSERDATA: {
                // Check if it's a Binary Ninja object with custom __tostring
                if (luaL_getmetafield(L, index, "__tostring") != LUA_TNIL) {
                    lua_pushvalue(L, index);
                    if (lua_pcall(L, 1, 1, 0) == LUA_OK) {
                        size_t len;
                        const char* result = lua_tolstring(L, -1, &len);
                        std::string ret = result ? std::string(result, len) : "<tostring failed>";
                        lua_pop(L, 1);
                        return ret;
                    }
                    lua_pop(L, 1); // Pop error
                }
                return std::string("<") + luaL_typename(L, index) + ">";
            }
            case LUA_TTABLE: {
                return "<table>";
            }
            case LUA_TFUNCTION: {
                return "<function>";
            }
            default: {
                return std::string("<") + luaL_typename(L, index) + ">";
            }
        }
    } catch (...) {
        return "<conversion error>";
    }
}


void LuaScriptingInstance::UpdateMagicVariableValues()
{
    if (!m_luaState) {
        m_logger->LogDebug("UpdateMagicVariableValues - no Lua state");
        return;
    }

    m_logger->LogDebug("UpdateMagicVariableValues - starting");

    try {
        sol::state_view lua(m_luaState);

        // Core context: current_function uses what UI provided
        if (m_currentFunction) {
            lua["current_function"] = m_currentFunction;
        } else {
            lua["current_function"] = sol::nil;
        }

        if (m_currentBinaryView) {
            lua["bv"] = m_currentBinaryView;
            lua["current_view"] = m_currentBinaryView;
        } else {
            lua["bv"] = sol::nil;
            lua["current_view"] = sol::nil;
        }

        if (m_currentBasicBlock) {
            lua["current_basic_block"] = m_currentBasicBlock;
        } else {
            lua["current_basic_block"] = sol::nil;
        }

        // Address variables - wrap in HexAddress for smartlink support
        lua["current_address"] = BinjaLua::HexAddress(m_currentAddress);
        lua["here"] = BinjaLua::HexAddress(m_currentAddress);

        // Selection
        lua["current_selection"] =
            BinjaLua::Selection(m_selectionBegin, m_selectionEnd);

        // Raw offset
        if (m_currentBinaryView) {
            uint64_t fileOffset;
            if (m_currentBinaryView->GetDataOffsetForAddress(
                    m_currentAddress, fileOffset)) {
                lua["current_raw_offset"] = BinjaLua::HexAddress(fileOffset);
            } else {
                lua["current_raw_offset"] = BinjaLua::HexAddress(0);
            }
        } else {
            lua["current_raw_offset"] = BinjaLua::HexAddress(0);
        }

        // Current sections
        if (m_currentBinaryView) {
            std::vector<Ref<Section>> sections =
                m_currentBinaryView->GetSectionsAt(m_currentAddress);
            sol::table sectionsTable = lua.create_table();
            for (size_t i = 0; i < sections.size(); i++) {
                sectionsTable[i + 1] = sections[i];
            }
            lua["current_sections"] = sectionsTable;
        } else {
            lua["current_sections"] = lua.create_table();
        }

        // Current symbol(s)
        if (m_currentBinaryView) {
            Ref<Symbol> symbol =
                m_currentBinaryView->GetSymbolByAddress(m_currentAddress);
            if (symbol) {
                lua["current_symbol"] = symbol;
                sol::table symbolsTable = lua.create_table();
                symbolsTable[1] = symbol;
                lua["current_symbols"] = symbolsTable;
            } else {
                lua["current_symbol"] = sol::nil;
                lua["current_symbols"] = lua.create_table();
            }
        } else {
            lua["current_symbol"] = sol::nil;
            lua["current_symbols"] = lua.create_table();
        }

        // Current comment
        if (m_currentBinaryView) {
            std::string comment =
                m_currentBinaryView->GetCommentForAddress(m_currentAddress);
            lua["current_comment"] = comment;
        } else {
            lua["current_comment"] = "";
        }

        // IL magic variables
        if (m_currentFunction) {
            lua["current_llil"] = m_currentFunction->GetLowLevelIL();
            lua["current_mlil"] = m_currentFunction->GetMediumLevelIL();
            lua["current_hlil"] = m_currentFunction->GetHighLevelIL();
        } else {
            lua["current_llil"] = sol::nil;
            lua["current_mlil"] = sol::nil;
            lua["current_hlil"] = sol::nil;
        }

        m_logger->LogDebug("UpdateMagicVariableValues - completed successfully");
    } catch (const std::exception& e) {
        m_logger->LogError("UpdateMagicVariableValues failed: %s", e.what());
    } catch (...) {
        m_logger->LogError("UpdateMagicVariableValues failed: Unknown error occurred");
    }
}

void LuaScriptingInstance::SetupUtilityFunctions()
{
    // dump() - pretty-print tables and values
    // Only define if not already defined (e.g., by lua-api extensions)
    lua_getglobal(m_luaState, "dump");
    bool dumpExists = !lua_isnil(m_luaState, -1);
    lua_pop(m_luaState, 1);

    if (!dumpExists) {
        const char* dumpCode =
            "function dump(o, indent)\n"
            "    indent = indent or 0\n"
            "    local sp = string.rep('  ', indent)\n"
            "    local sp1 = string.rep('  ', indent + 1)\n"
            "    if type(o) == 'table' then\n"
            "        local parts = {'{'}\n"
            "        local n = #o\n"
            "        if n > 0 then\n"
            "            for i = 1, n do\n"
            "                parts[#parts+1] = sp1 .. dump(o[i], indent+1) .. (i < n and ',' or '')\n"
            "            end\n"
            "        else\n"
            "            local keys = {}\n"
            "            for k in pairs(o) do keys[#keys+1] = k end\n"
            "            table.sort(keys, function(a,b) return tostring(a) < tostring(b) end)\n"
            "            for i, k in ipairs(keys) do\n"
            "                local ks = type(k) == 'string' and k:match('^[%a_][%w_]*$') and k or ('['..dump(k,0)..']')\n"
            "                parts[#parts+1] = sp1 .. ks .. ' = ' .. dump(o[k], indent+1) .. (i < #keys and ',' or '')\n"
            "            end\n"
            "        end\n"
            "        parts[#parts+1] = sp .. '}'\n"
            "        return table.concat(parts, '\\n')\n"
            "    elseif type(o) == 'string' then\n"
            "        return '\"' .. o:gsub('\\\\', '\\\\\\\\'):gsub('\"', '\\\\\"'):gsub('\\n', '\\\\n') .. '\"'\n"
            "    elseif type(o) == 'number' and o > 4096 and o == math.floor(o) then\n"
            "        return string.format('0x%x', o)\n"
            "    else\n"
            "        return tostring(o)\n"
            "    end\n"
            "end\n";
        int result = luaL_dostring(m_luaState, dumpCode);
        if (result != LUA_OK) {
            m_logger->LogError("Failed to register dump function: %s",
                lua_tostring(m_luaState, -1));
            lua_pop(m_luaState, 1);
        }
    }

    // get_selected_data()
    lua_pushcfunction(m_luaState, [](lua_State* L) -> int {
        lua_getglobal(L, "__binja_instance");
        LuaScriptingInstance* instance = static_cast<LuaScriptingInstance*>(lua_touserdata(L, -1));
        lua_pop(L, 1);

        if (!instance || !instance->m_currentBinaryView) {
            lua_pushstring(L, "");
            return 1;
        }

        uint64_t start = instance->m_selectionBegin;
        uint64_t end = instance->m_selectionEnd;
        if (start >= end) {
            lua_pushstring(L, "");
            return 1;
        }

        size_t len = end - start;
        DataBuffer data = instance->m_currentBinaryView->ReadBuffer(start, len);
        lua_pushlstring(L, (const char*)data.GetData(), data.GetLength());
        return 1;
    });
    lua_setglobal(m_luaState, "get_selected_data");

    // write_at_cursor(data)
    lua_pushcfunction(m_luaState, [](lua_State* L) -> int {
        if (lua_gettop(L) < 1) {
            lua_pushboolean(L, false);
            return 1;
        }

        lua_getglobal(L, "__binja_instance");
        LuaScriptingInstance* instance = static_cast<LuaScriptingInstance*>(lua_touserdata(L, -1));
        lua_pop(L, 1);

        if (!instance || !instance->m_currentBinaryView) {
            lua_pushboolean(L, false);
            return 1;
        }

        size_t dataLen;
        const char* data = lua_tolstring(L, 1, &dataLen);
        if (!data) {
            lua_pushboolean(L, false);
            return 1;
        }

        uint64_t address = instance->m_selectionBegin;
        size_t written = instance->m_currentBinaryView->WriteBuffer(address, DataBuffer(data, dataLen));
        lua_pushboolean(L, written == dataLen);
        return 1;
    });
    lua_setglobal(m_luaState, "write_at_cursor");
}

bool LuaScriptingInstance::HasContextChanged() const
{
    // Check if any of the context variables have changed since last cache update
    return (m_currentBinaryView != m_lastCachedBinaryView ||
            m_currentFunction != m_lastCachedFunction ||
            m_currentBasicBlock != m_lastCachedBasicBlock ||
            m_currentAddress != m_lastCachedAddress ||
            m_selectionBegin != m_lastCachedSelectionBegin ||
            m_selectionEnd != m_lastCachedSelectionEnd);
}

void LuaScriptingInstance::UpdateContextCache()
{
    // Update the cached context to match current state
    m_lastCachedBinaryView = m_currentBinaryView;
    m_lastCachedFunction = m_currentFunction;
    m_lastCachedBasicBlock = m_currentBasicBlock;
    m_lastCachedAddress = m_currentAddress;
    m_lastCachedSelectionBegin = m_selectionBegin;
    m_lastCachedSelectionEnd = m_selectionEnd;
}