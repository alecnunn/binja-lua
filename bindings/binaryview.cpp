// Sol2 BinaryView bindings for binja-lua

#include "common.h"
#include <cmath>

namespace BinjaLua {

void RegisterBinaryViewBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering BinaryView bindings");

    lua.new_usertype<BinaryView>(BINARYVIEW_METATABLE,
        sol::no_constructor,

        // Properties - addresses return HexAddress for consistency
        "start_addr", sol::property([](BinaryView& bv) -> HexAddress {
            return HexAddress(bv.GetStart());
        }),

        "end_addr", sol::property([](BinaryView& bv) -> HexAddress {
            return HexAddress(bv.GetEnd());
        }),

        "length", sol::property([](BinaryView& bv) -> uint64_t {
            return bv.GetLength();
        }),

        "filename", sol::property([](BinaryView& bv) -> std::string {
            Ref<FileMetadata> file = bv.GetFile();
            return file ? file->GetFilename() : "<unknown>";
        }),

        "arch", sol::property([](BinaryView& bv) -> Ref<Architecture> {
            return bv.GetDefaultArchitecture();
        }),

        "platform", sol::property([](BinaryView& bv) -> Ref<Platform> {
            return bv.GetDefaultPlatform();
        }),

        "entry_point", sol::property([](BinaryView& bv) -> HexAddress {
            return HexAddress(bv.GetEntryPoint());
        }),

        // Collection methods - use method syntax: bv:functions(), bv:sections(), etc.
        // Note: sol::property with sol::this_state causes crashes, so these are methods
        "functions", [](sol::this_state ts, BinaryView& bv) -> sol::table {
            return ToLuaTable(ts, bv.GetAnalysisFunctionList());
        },

        "sections", [](sol::this_state ts, BinaryView& bv) -> sol::table {
            return ToLuaTable(ts, bv.GetSections());
        },

        "strings", [](sol::this_state ts, BinaryView& bv) -> sol::table {
            sol::state_view lua(ts);
            std::vector<BNStringReference> strs = bv.GetStrings();
            sol::table result = lua.create_table();
            for (size_t i = 0; i < strs.size(); i++) {
                sol::table entry = lua.create_table();
                entry["addr"] = HexAddress(strs[i].start);
                entry["length"] = strs[i].length;
                entry["type"] = strs[i].type;
                if (strs[i].length > 0) {
                    DataBuffer data = bv.ReadBuffer(strs[i].start, strs[i].length);
                    if (data.GetLength() > 0) {
                        entry["value"] = std::string((const char*)data.GetData(),
                            std::min((size_t)strs[i].length, data.GetLength()));
                    }
                }
                result[i + 1] = entry;
            }
            return result;
        },

        "imports", [](sol::this_state ts, BinaryView& bv) -> sol::table {
            sol::state_view lua(ts);
            std::vector<Ref<Symbol>> all = bv.GetSymbols();
            sol::table result = lua.create_table();
            int idx = 1;
            for (const auto& sym : all) {
                BNSymbolType t = sym->GetType();
                if (t == ImportAddressSymbol || t == ImportedFunctionSymbol || t == ImportedDataSymbol)
                    result[idx++] = sym;
            }
            return result;
        },

        "exports", [](sol::this_state ts, BinaryView& bv) -> sol::table {
            sol::state_view lua(ts);
            std::vector<Ref<Symbol>> all = bv.GetSymbols();
            sol::table result = lua.create_table();
            int idx = 1;
            for (const auto& sym : all) {
                BNSymbolType t = sym->GetType();
                if (t == FunctionSymbol || t == DataSymbol) result[idx++] = sym;
            }
            return result;
        },

        // Symbol CRUD
        "define_user_symbol", [](BinaryView& bv, Ref<Symbol> sym) {
            if (sym) bv.DefineUserSymbol(sym);
        },

        "define_auto_symbol", [](BinaryView& bv, Ref<Symbol> sym) {
            if (sym) bv.DefineAutoSymbol(sym);
        },

        "undefine_user_symbol", [](BinaryView& bv, Ref<Symbol> sym) {
            if (sym) bv.UndefineUserSymbol(sym);
        },

        "undefine_auto_symbol", [](BinaryView& bv, Ref<Symbol> sym) {
            if (sym) bv.UndefineAutoSymbol(sym);
        },

        "get_symbol_at", [](BinaryView& bv, sol::object addr_obj)
            -> Ref<Symbol> {
            auto addr = AsAddress(addr_obj);
            if (!addr) return nullptr;
            return bv.GetSymbolByAddress(*addr);
        },

        "get_symbols_by_name", [](sol::this_state ts, BinaryView& bv,
                                  const std::string& name) -> sol::table {
            return ToLuaTable(ts, bv.GetSymbolsByName(name));
        },

        "get_symbols_of_type", [](sol::this_state ts, BinaryView& bv,
                                  const std::string& type_name) -> sol::table {
            sol::state_view lua(ts);
            auto type = EnumFromString<BNSymbolType>(type_name);
            if (!type) {
                return lua.create_table();
            }
            return ToLuaTable(ts, bv.GetSymbolsOfType(*type));
        },

        // Data variable properties
        "has_data_vars", sol::property([](BinaryView& bv) -> bool {
            return bv.HasDataVariables();
        }),

        // Note: sol::property with sol::this_state causes crashes, use method syntax
        "data_vars", [](sol::this_state ts, BinaryView& bv) -> sol::table {
            sol::state_view lua(ts);
            std::map<uint64_t, DataVariable> vars = bv.GetDataVariables();
            sol::table result = lua.create_table();
            Ref<BinaryView> bvRef = &bv;
            int idx = 1;
            for (const auto& [addr, var] : vars) {
                result[idx++] = DataVariableWrapper(
                    var.address, bvRef, var.type, var.autoDiscovered);
            }
            return result;
        },

        // Lookup methods - accept both uint64_t and HexAddress
        "get_function_at", [](BinaryView& bv, sol::object addr_obj) -> Ref<Function> {
            auto addr = AsAddress(addr_obj);
            if (!addr) return nullptr;
            return bv.GetAnalysisFunction(bv.GetDefaultPlatform(), *addr);
        },

        // Data variable methods
        "get_data_var_at", [](BinaryView& bv, uint64_t addr)
            -> std::optional<DataVariableWrapper> {
            DataVariable var;
            if (bv.GetDataVariableAtAddress(addr, var)) {
                Ref<BinaryView> bvRef = &bv;
                return DataVariableWrapper(var.address, bvRef, var.type,
                                           var.autoDiscovered);
            }
            return std::nullopt;
        },

        "define_data_var",
        [](BinaryView& bv, uint64_t addr, const std::string& typeStr,
           sol::optional<std::string> name) -> bool {
            // Parse type string
            QualifiedNameAndType result;
            std::string errors;
            if (!bv.ParseTypeString(typeStr, result, errors)) {
                return false;
            }
            Confidence<Ref<Type>> typeConf(result.type, 255);
            bv.DefineDataVariable(addr, typeConf);

            // Define symbol if name provided
            if (name && !name->empty()) {
                Ref<Symbol> sym =
                    new Symbol(DataSymbol, *name, *name, *name, addr);
                bv.DefineUserSymbol(sym);
            }
            return true;
        },

        "define_user_data_var",
        [](BinaryView& bv, uint64_t addr, const std::string& typeStr,
           sol::optional<std::string> name) -> std::optional<DataVariableWrapper> {
            // Parse type string
            QualifiedNameAndType result;
            std::string errors;
            if (!bv.ParseTypeString(typeStr, result, errors)) {
                return std::nullopt;
            }
            Confidence<Ref<Type>> typeConf(result.type, 255);
            bv.DefineUserDataVariable(addr, typeConf);

            // Define symbol if name provided
            if (name && !name->empty()) {
                Ref<Symbol> sym =
                    new Symbol(DataSymbol, *name, *name, *name, addr);
                bv.DefineUserSymbol(sym);
            }

            // Return the newly created data variable
            DataVariable var;
            if (bv.GetDataVariableAtAddress(addr, var)) {
                Ref<BinaryView> bvRef = &bv;
                return DataVariableWrapper(var.address, bvRef, var.type,
                                           var.autoDiscovered);
            }
            return std::nullopt;
        },

        "undefine_data_var", [](BinaryView& bv, uint64_t addr) {
            bv.UndefineDataVariable(addr);
        },

        "undefine_user_data_var", [](BinaryView& bv, uint64_t addr) {
            bv.UndefineUserDataVariable(addr);
        },

        "get_next_data_var_after", [](BinaryView& bv, uint64_t addr)
            -> std::optional<DataVariableWrapper> {
            uint64_t nextAddr =
                BNGetNextDataVariableStartAfterAddress(bv.GetObject(), addr);
            if (nextAddr == 0 || nextAddr == addr) {
                return std::nullopt;
            }
            DataVariable var;
            if (bv.GetDataVariableAtAddress(nextAddr, var)) {
                Ref<BinaryView> bvRef = &bv;
                return DataVariableWrapper(var.address, bvRef, var.type,
                                           var.autoDiscovered);
            }
            return std::nullopt;
        },

        "get_previous_data_var_before", [](BinaryView& bv, uint64_t addr)
            -> std::optional<DataVariableWrapper> {
            uint64_t prevAddr =
                BNGetPreviousDataVariableStartBeforeAddress(bv.GetObject(), addr);
            if (prevAddr == 0 || prevAddr == addr) {
                return std::nullopt;
            }
            DataVariable var;
            if (bv.GetDataVariableAtAddress(prevAddr, var)) {
                Ref<BinaryView> bvRef = &bv;
                return DataVariableWrapper(var.address, bvRef, var.type,
                                           var.autoDiscovered);
            }
            return std::nullopt;
        },

        // Navigation functions - all accept HexAddress or uint64_t
        "get_next_function_start_after", [](BinaryView& bv, sol::object addr_obj)
            -> std::optional<HexAddress> {
            auto addr = AsAddress(addr_obj);
            if (!addr) return std::nullopt;
            uint64_t nextAddr = bv.GetNextFunctionStartAfterAddress(*addr);
            if (nextAddr == 0 || nextAddr <= *addr) return std::nullopt;
            return HexAddress(nextAddr);
        },

        "get_previous_function_start_before", [](BinaryView& bv, sol::object addr_obj)
            -> std::optional<HexAddress> {
            auto addr = AsAddress(addr_obj);
            if (!addr) return std::nullopt;
            uint64_t prevAddr = bv.GetPreviousFunctionStartBeforeAddress(*addr);
            if (prevAddr == 0 || prevAddr >= *addr) return std::nullopt;
            return HexAddress(prevAddr);
        },

        "get_next_basic_block_start_after", [](BinaryView& bv, sol::object addr_obj)
            -> std::optional<HexAddress> {
            auto addr = AsAddress(addr_obj);
            if (!addr) return std::nullopt;
            uint64_t nextAddr = bv.GetNextBasicBlockStartAfterAddress(*addr);
            if (nextAddr == 0 || nextAddr <= *addr) return std::nullopt;
            return HexAddress(nextAddr);
        },

        "get_previous_basic_block_start_before", [](BinaryView& bv, sol::object addr_obj)
            -> std::optional<HexAddress> {
            auto addr = AsAddress(addr_obj);
            if (!addr) return std::nullopt;
            uint64_t prevAddr = bv.GetPreviousBasicBlockStartBeforeAddress(*addr);
            if (prevAddr == 0 || prevAddr >= *addr) return std::nullopt;
            return HexAddress(prevAddr);
        },

        "get_next_data_after", [](BinaryView& bv, sol::object addr_obj)
            -> std::optional<HexAddress> {
            auto addr = AsAddress(addr_obj);
            if (!addr) return std::nullopt;
            uint64_t nextAddr = bv.GetNextDataAfterAddress(*addr);
            if (nextAddr == 0 || nextAddr <= *addr) return std::nullopt;
            return HexAddress(nextAddr);
        },

        "get_previous_data_before", [](BinaryView& bv, sol::object addr_obj)
            -> std::optional<HexAddress> {
            auto addr = AsAddress(addr_obj);
            if (!addr) return std::nullopt;
            uint64_t prevAddr = bv.GetPreviousDataBeforeAddress(*addr);
            if (prevAddr == 0 || prevAddr >= *addr) return std::nullopt;
            return HexAddress(prevAddr);
        },

        // Advanced lookup functions - all accept HexAddress or uint64_t
        "get_functions_at", [](sol::this_state ts, BinaryView& bv, sol::object addr_obj)
            -> sol::table {
            auto addr = AsAddress(addr_obj);
            if (!addr) return sol::state_view(ts).create_table();
            return ToLuaTable(ts, bv.GetAnalysisFunctionsForAddress(*addr));
        },

        "get_functions_containing", [](sol::this_state ts, BinaryView& bv, sol::object addr_obj)
            -> sol::table {
            auto addr = AsAddress(addr_obj);
            if (!addr) return sol::state_view(ts).create_table();
            return ToLuaTable(ts, bv.GetAnalysisFunctionsContainingAddress(*addr));
        },

        "get_basic_blocks_starting_at", [](sol::this_state ts, BinaryView& bv, sol::object addr_obj)
            -> sol::table {
            auto addr = AsAddress(addr_obj);
            if (!addr) return sol::state_view(ts).create_table();
            return ToLuaTable(ts, bv.GetBasicBlocksStartingAtAddress(*addr));
        },

        // Search by function name
        "get_functions_by_name", [](sol::this_state ts, BinaryView& bv,
                                    const std::string& name,
                                    sol::optional<bool> exact) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            bool exactMatch = exact.value_or(true);

            std::vector<Ref<Function>> allFuncs = bv.GetAnalysisFunctionList();
            int idx = 1;
            for (const auto& func : allFuncs) {
                std::string funcName = func->GetSymbol()->GetShortName();
                bool match = false;
                if (exactMatch) {
                    match = (funcName == name);
                } else {
                    match = (funcName.find(name) != std::string::npos);
                }
                if (match) {
                    result[idx++] = func;
                }
            }
            return result;
        },

        // Search functions
        "find_next_data", [](BinaryView& bv, sol::object start_obj,
                            const std::string& data) -> std::optional<HexAddress> {
            auto start = AsAddress(start_obj);
            if (!start) return std::nullopt;

            DataBuffer searchData(data.data(), data.size());
            uint64_t resultAddr = 0;
            bool found = bv.FindNextData(*start, searchData, resultAddr);
            if (!found) return std::nullopt;
            return HexAddress(resultAddr);
        },

        "find_all_data", [](sol::this_state ts, BinaryView& bv,
                           sol::object start_obj, sol::object end_obj,
                           const std::string& data) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();

            auto start = AsAddress(start_obj);
            auto end = AsAddress(end_obj);
            if (!start || !end) return result;

            DataBuffer searchData(data.data(), data.size());
            uint64_t current = *start;
            int idx = 1;

            while (current < *end) {
                uint64_t foundAddr = 0;
                bool found = bv.FindNextData(current, searchData, foundAddr);
                if (!found || foundAddr >= *end) break;
                result[idx++] = HexAddress(foundAddr);
                current = foundAddr + 1;
            }
            return result;
        },

        "find_next_text", [](sol::this_state ts, BinaryView& bv, sol::object start_obj,
                            const std::string& pattern) -> sol::object {
            sol::state_view lua(ts);
            auto start = AsAddress(start_obj);
            if (!start) return sol::make_object(lua, sol::nil);

            // Search for the text pattern as raw bytes
            DataBuffer searchData(pattern.data(), pattern.size());
            uint64_t resultAddr = 0;
            bool found = bv.FindNextData(*start, searchData, resultAddr);
            if (!found) {
                return sol::make_object(lua, sol::nil);
            }
            return sol::make_object(lua, HexAddress(resultAddr));
        },

        "find_all_text", [](sol::this_state ts, BinaryView& bv,
                           sol::object start_obj, sol::object end_obj,
                           const std::string& pattern) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();

            auto start = AsAddress(start_obj);
            auto end = AsAddress(end_obj);
            if (!start || !end) return result;

            DataBuffer searchData(pattern.data(), pattern.size());
            uint64_t current = *start;
            int idx = 1;

            while (current < *end) {
                uint64_t foundAddr = 0;
                bool found = bv.FindNextData(current, searchData, foundAddr);
                if (!found || foundAddr >= *end) break;

                sol::table entry = lua.create_table();
                entry["addr"] = HexAddress(foundAddr);
                entry["match"] = pattern;
                result[idx++] = entry;
                current = foundAddr + 1;
            }
            return result;
        },

        "find_next_constant", [](BinaryView& bv, sol::object start_obj,
                                uint64_t constant) -> std::optional<HexAddress> {
            auto start = AsAddress(start_obj);
            if (!start) return std::nullopt;

            uint64_t resultAddr = 0;
            Ref<DisassemblySettings> settings = DisassemblySettings::GetDefaultSettings();
            bool found = bv.FindNextConstant(*start, constant, resultAddr, settings);
            if (!found) return std::nullopt;
            return HexAddress(resultAddr);
        },

        "read", [](BinaryView& bv, uint64_t addr, size_t len) -> std::string {
            DataBuffer buf = bv.ReadBuffer(addr, len);
            return std::string((const char*)buf.GetData(), buf.GetLength());
        },

        "get_code_refs", [](sol::this_state ts, BinaryView& bv, uint64_t addr) -> sol::table {
            return ReferenceSourcesToTable(ts, bv.GetCodeReferences(addr));
        },

        "get_data_refs", [](sol::this_state ts, BinaryView& bv, uint64_t addr) -> sol::table {
            return ToLuaTable(ts, bv.GetDataReferences(addr),
                              [](uint64_t a) { return HexAddress(a); });
        },

        // Cross-reference "from" methods (what does this address reference?)
        "get_code_refs_from", [](sol::this_state ts, BinaryView& bv, uint64_t addr)
            -> sol::table {
            sol::state_view lua(ts);
            // Create ReferenceSource for the query
            ReferenceSource src;
            src.addr = addr;
            src.arch = bv.GetDefaultArchitecture();
            src.func = bv.GetAnalysisFunction(bv.GetDefaultPlatform(), addr);
            std::vector<uint64_t> refs = bv.GetCodeReferencesFrom(src);
            sol::table result = lua.create_table();
            for (size_t i = 0; i < refs.size(); i++) {
                result[i + 1] = HexAddress(refs[i]);
            }
            return result;
        },

        "get_data_refs_from", [](sol::this_state ts, BinaryView& bv, uint64_t addr)
            -> sol::table {
            sol::state_view lua(ts);
            std::vector<uint64_t> refs = bv.GetDataReferencesFrom(addr);
            sol::table result = lua.create_table();
            for (size_t i = 0; i < refs.size(); i++) {
                result[i + 1] = HexAddress(refs[i]);
            }
            return result;
        },

        // Caller/callee methods
        "get_callers", [](sol::this_state ts, BinaryView& bv, uint64_t addr) -> sol::table {
            return ReferenceSourcesToTable(ts, bv.GetCallers(addr));
        },

        "get_callees", [](sol::this_state ts, BinaryView& bv, uint64_t addr) -> sol::table {
            sol::state_view lua(ts);
            // Create ReferenceSource for the query
            ReferenceSource src;
            src.addr = addr;
            src.arch = bv.GetDefaultArchitecture();
            src.func = bv.GetAnalysisFunction(bv.GetDefaultPlatform(), addr);
            std::vector<uint64_t> refs = bv.GetCallees(src);
            sol::table result = lua.create_table();
            for (size_t i = 0; i < refs.size(); i++) {
                result[i + 1] = HexAddress(refs[i]);
            }
            return result;
        },

        // Comment methods
        "comment_at_address", [](BinaryView& bv, uint64_t addr) -> std::string {
            return bv.GetCommentForAddress(addr);
        },

        "set_comment_at_address", [](BinaryView& bv, uint64_t addr, const std::string& comment) -> bool {
            bv.SetCommentForAddress(addr, comment);
            return true;
        },

        // Tag type management
        "tag_types", [](sol::this_state ts, BinaryView& bv) -> sol::table {
            return ToLuaTable(ts, bv.GetTagTypes());
        },

        "get_tag_type", [](BinaryView& bv, const std::string& name) -> Ref<TagType> {
            return bv.GetTagType(name);
        },

        "create_tag_type", [](BinaryView& bv, const std::string& name,
                              const std::string& icon) -> Ref<TagType> {
            Ref<TagType> tagType = new TagType(&bv, name, icon);
            bv.AddTagType(tagType);
            return tagType;
        },

        "remove_tag_type", [](BinaryView& bv, Ref<TagType> tagType) {
            bv.RemoveTagType(tagType);
        },

        // Tag operations at addresses
        "get_tags_at", [](sol::this_state ts, BinaryView& bv, sol::object addr_obj)
            -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto addr = AsAddress(addr_obj);
            if (!addr) return result;
            Ref<Architecture> arch = bv.GetDefaultArchitecture();
            std::vector<Ref<Tag>> tags = bv.GetDataTags(*addr);
            for (size_t i = 0; i < tags.size(); i++) {
                result[i + 1] = tags[i];
            }
            return result;
        },

        "add_tag", [](BinaryView& bv, sol::object addr_obj, Ref<Tag> tag,
                      sol::optional<bool> user) {
            auto addr = AsAddress(addr_obj);
            if (!addr) return;
            bool isUser = user.value_or(true);
            if (isUser) {
                bv.AddUserDataTag(*addr, tag);
            } else {
                bv.AddAutoDataTag(*addr, tag);
            }
        },

        "remove_tag", [](BinaryView& bv, sol::object addr_obj, Ref<Tag> tag,
                         sol::optional<bool> user) {
            auto addr = AsAddress(addr_obj);
            if (!addr) return;
            bool isUser = user.value_or(true);
            if (isUser) {
                bv.RemoveUserDataTag(*addr, tag);
            } else {
                bv.RemoveAutoDataTag(*addr, tag);
            }
        },

        "create_user_tag", [](BinaryView& bv, sol::object addr_obj,
                              const std::string& tagTypeName,
                              const std::string& data) -> Ref<Tag> {
            auto addr = AsAddress(addr_obj);
            if (!addr) return nullptr;
            return bv.CreateUserDataTag(*addr, tagTypeName, data);
        },

        // Get all tags
        "get_all_tags", [](sol::this_state ts, BinaryView& bv) -> sol::table {
            sol::state_view lua(ts);
            std::vector<TagReference> refs = bv.GetAllTagReferences();
            sol::table result = lua.create_table();
            for (size_t i = 0; i < refs.size(); i++) {
                sol::table entry = lua.create_table();
                entry["addr"] = HexAddress(refs[i].addr);
                entry["tag"] = refs[i].tag;
                entry["auto"] = refs[i].autoDefined;
                if (refs[i].func) entry["func"] = refs[i].func;
                result[i + 1] = entry;
            }
            return result;
        },

        "get_tags_of_type", [](sol::this_state ts, BinaryView& bv, Ref<TagType> tagType)
            -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            if (!tagType) return result;
            std::vector<TagReference> refs = bv.GetAllTagReferencesOfType(tagType);
            for (size_t i = 0; i < refs.size(); i++) {
                sol::table entry = lua.create_table();
                entry["addr"] = HexAddress(refs[i].addr);
                entry["tag"] = refs[i].tag;
                entry["auto"] = refs[i].autoDefined;
                if (refs[i].func) entry["func"] = refs[i].func;
                result[i + 1] = entry;
            }
            return result;
        },

        // Get tags in an address range
        "get_tags_in_range", [](sol::this_state ts, BinaryView& bv,
                                sol::object start_obj, sol::object end_obj,
                                sol::optional<bool> user_only) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto start = AsAddress(start_obj);
            auto end = AsAddress(end_obj);
            if (!start || !end) return result;
            std::vector<TagReference> refs;
            if (user_only.has_value()) {
                if (user_only.value()) {
                    refs = bv.GetUserDataTagsInRange(*start, *end);
                } else {
                    refs = bv.GetAutoDataTagsInRange(*start, *end);
                }
            } else {
                refs = bv.GetDataTagsInRange(*start, *end);
            }
            for (size_t i = 0; i < refs.size(); i++) {
                sol::table entry = lua.create_table();
                entry["addr"] = HexAddress(refs[i].addr);
                entry["tag"] = refs[i].tag;
                entry["auto"] = refs[i].autoDefined;
                if (refs[i].func) entry["func"] = refs[i].func;
                result[i + 1] = entry;
            }
            return result;
        },

        // Transaction/undo methods
        "run_transaction", [](sol::this_state ts, BinaryView& bv, sol::function func) -> bool {
            sol::state_view lua(ts);
            return bv.RunUndoableTransaction([&func, &lua]() -> bool {
                sol::protected_function_result result = func();
                if (!result.valid()) {
                    sol::error err = result;
                    return false;
                }
                return result.get<bool>();
            });
        },

        "can_undo", [](BinaryView& bv) -> bool { return bv.CanUndo(); },
        "undo", [](BinaryView& bv) -> bool { return bv.Undo(); },
        "can_redo", [](BinaryView& bv) -> bool { return bv.CanRedo(); },
        "redo", [](BinaryView& bv) -> bool { return bv.Redo(); },

        // Type system methods
        "get_type_by_name", [](BinaryView& bv, const std::string& name) -> Ref<Type> {
            QualifiedName qname(name);
            return bv.GetTypeByName(qname);
        },

        "get_type_by_id", [](BinaryView& bv, const std::string& id) -> Ref<Type> {
            return bv.GetTypeById(id);
        },

        "get_type_id", [](BinaryView& bv, const std::string& name) -> std::string {
            QualifiedName qname(name);
            return bv.GetTypeId(qname);
        },

        "types", [](sol::this_state ts, BinaryView& bv) -> sol::table {
            sol::state_view lua(ts);
            std::map<QualifiedName, Ref<Type>> types = bv.GetTypes();
            sol::table result = lua.create_table();
            int i = 1;
            for (const auto& [name, type] : types) {
                sol::table entry = lua.create_table();
                entry["name"] = name.GetString();
                entry["type"] = type;
                result[i++] = entry;
            }
            return result;
        },

        "define_user_type", [](BinaryView& bv, const std::string& name,
                               const std::string& typeStr) -> bool {
            QualifiedNameAndType result;
            std::string errors;
            if (!bv.ParseTypeString(typeStr, result, errors)) {
                return false;
            }
            QualifiedName qname(name);
            bv.DefineUserType(qname, result.type);
            return true;
        },

        "undefine_user_type", [](BinaryView& bv, const std::string& name) {
            QualifiedName qname(name);
            bv.UndefineUserType(qname);
        },

        "parse_type_string",
        [](sol::this_state ts, BinaryView& bv, const std::string& typeStr)
            -> std::tuple<sol::object, std::string> {
            sol::state_view lua(ts);
            QualifiedNameAndType result;
            std::string errors;
            if (!bv.ParseTypeString(typeStr, result, errors)) {
                return {sol::make_object(lua, sol::nil), errors};
            }
            return {sol::make_object(lua, result.type), ""};
        },

        // Type reference queries
        "get_type_refs_for_type", [](sol::this_state ts, BinaryView& bv,
                                     const std::string& typeName) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();

            QualifiedName qname(typeName);
            std::vector<TypeReferenceSource> refs = bv.GetTypeReferencesForType(qname);

            for (size_t i = 0; i < refs.size(); i++) {
                sol::table entry = lua.create_table();
                entry["name"] = refs[i].name.GetString();
                entry["offset"] = refs[i].offset;

                // Convert type to string
                std::string refType;
                switch (refs[i].type) {
                    case BNTypeReferenceType::DirectTypeReferenceType: refType = "direct"; break;
                    case BNTypeReferenceType::IndirectTypeReferenceType: refType = "indirect"; break;
                    default: refType = "unknown"; break;
                }
                entry["ref_type"] = refType;

                result[i + 1] = entry;
            }
            return result;
        },

        "get_outgoing_type_refs", [](sol::this_state ts, BinaryView& bv,
                                     const std::string& typeName) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();

            QualifiedName qname(typeName);
            auto refs = bv.GetOutgoingDirectTypeReferences(qname);

            int idx = 1;
            for (const auto& ref : refs) {
                result[idx++] = ref.GetString();
            }
            return result;
        },

        "get_outgoing_recursive_type_refs", [](sol::this_state ts, BinaryView& bv,
                                               const std::string& typeName) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();

            QualifiedName qname(typeName);
            auto refs = bv.GetOutgoingRecursiveTypeReferences(qname);

            int idx = 1;
            for (const auto& ref : refs) {
                result[idx++] = ref.GetString();
            }
            return result;
        },

        // ============================================================
        // Analysis Control
        // ============================================================

        // Trigger analysis update (non-blocking)
        "update_analysis", [](BinaryView& bv) {
            bv.UpdateAnalysis();
        },

        // Update analysis and wait for completion
        "update_analysis_and_wait", [](BinaryView& bv) {
            bv.UpdateAnalysisAndWait();
        },

        // Abort ongoing analysis
        "abort_analysis", [](BinaryView& bv) {
            bv.AbortAnalysis();
        },

        // Get analysis progress as a method
        "get_analysis_progress", [](sol::this_state ts, BinaryView& bv) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();

            BNAnalysisProgress progress = bv.GetAnalysisProgress();

            result["state"] = EnumToString(progress.state);
            result["count"] = static_cast<int64_t>(progress.count);
            result["total"] = static_cast<int64_t>(progress.total);

            return result;
        },

        // ============================================================
        // Metadata System
        // ============================================================

        // Store metadata (supports bool, string, integer, number, table)
        "store_metadata", [](sol::this_state ts, BinaryView& bv,
                            const std::string& key, sol::object value,
                            sol::optional<bool> isAuto) {
            (void)ts;
            BNMetadata* md = MetadataFromLua(value);
            if (md) {
                BNBinaryViewStoreMetadata(BinaryView::GetObject(&bv),
                                         key.c_str(), md,
                                         isAuto.value_or(false));
                BNFreeMetadata(md);
            }
        },

        // Query metadata
        "query_metadata", [](sol::this_state ts, BinaryView& bv,
                            const std::string& key) -> sol::object {
            sol::state_view lua(ts);
            BNMetadata* md = BNBinaryViewQueryMetadata(
                BinaryView::GetObject(&bv), key.c_str());
            if (!md) {
                return sol::make_object(lua, sol::nil);
            }
            sol::object result = MetadataToLua(lua, md);
            BNFreeMetadata(md);
            return result;
        },

        // Remove metadata
        "remove_metadata", [](BinaryView& bv, const std::string& key) {
            BNBinaryViewRemoveMetadata(BinaryView::GetObject(&bv), key.c_str());
        },

        // ============================================================
        // Reports
        // ============================================================

        // Show plain text report
        "show_plain_text_report", [](BinaryView& bv, const std::string& title,
                                     const std::string& contents) {
            BNShowPlainTextReport(BinaryView::GetObject(&bv), title.c_str(), contents.c_str());
        },

        // Show markdown report
        "show_markdown_report", [](BinaryView& bv, const std::string& title,
                                   const std::string& contents,
                                   sol::optional<std::string> plaintext) {
            BNShowMarkdownReport(BinaryView::GetObject(&bv), title.c_str(), contents.c_str(),
                                plaintext.value_or("").c_str());
        },

        // Show HTML report
        "show_html_report", [](BinaryView& bv, const std::string& title,
                               const std::string& contents,
                               sol::optional<std::string> plaintext) {
            BNShowHTMLReport(BinaryView::GetObject(&bv), title.c_str(), contents.c_str(),
                            plaintext.value_or("").c_str());
        },

        // Show graph report (FlowGraph)
        "show_graph_report", [](BinaryView& bv, const std::string& title,
                                FlowGraph& graph) {
            BNShowGraphReport(BinaryView::GetObject(&bv), title.c_str(),
                             FlowGraph::GetObject(&graph));
        },

        // Comparison
        sol::meta_function::equal_to, [](BinaryView& a, BinaryView& b) -> bool {
            return BinaryView::GetObject(&a) == BinaryView::GetObject(&b);
        },

        // String representation
        sol::meta_function::to_string, [](BinaryView& bv) -> std::string {
            Ref<FileMetadata> file = bv.GetFile();
            std::string name = file ? file->GetFilename() : "<unknown>";
            return fmt::format("<BinaryView: '{}', start 0x{:x}, len 0x{:x}>",
                             name, bv.GetStart(), bv.GetLength());
        }
    );

    if (logger) logger->LogDebug("BinaryView bindings registered");
}

} // namespace BinjaLua
