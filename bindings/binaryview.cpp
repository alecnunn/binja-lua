// Sol2 BinaryView bindings for binja-lua

#include "common.h"
#include <cmath>

namespace BinjaLua {

// Helper to extract uint64_t from sol::object (HexAddress or numeric)
static std::optional<uint64_t> GetAddressFromObject(sol::object obj) {
    if (obj.is<HexAddress>()) {
        return obj.as<HexAddress>().value;
    } else if (obj.is<uint64_t>()) {
        return obj.as<uint64_t>();
    } else if (obj.is<int64_t>()) {
        return static_cast<uint64_t>(obj.as<int64_t>());
    } else if (obj.is<double>()) {
        return static_cast<uint64_t>(obj.as<double>());
    }
    return std::nullopt;
}

void RegisterBinaryViewBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering BinaryView bindings");

    lua.new_usertype<BinaryView>(BINARYVIEW_METATABLE,
        sol::no_constructor,

        // Properties - addresses return HexAddress for consistency
        "start_addr", sol::property([](BinaryView& bv) -> HexAddress {
            return HexAddress(bv.GetStart());
        }),
        // Alias for start_addr
        "start", sol::property([](BinaryView& bv) -> HexAddress {
            return HexAddress(bv.GetStart());
        }),

        "end_addr", sol::property([](BinaryView& bv) -> HexAddress {
            return HexAddress(bv.GetEnd());
        }),
        // Alias for end_addr (note: "end" is a Lua keyword, use bv["end"])
        "end", sol::property([](BinaryView& bv) -> HexAddress {
            return HexAddress(bv.GetEnd());
        }),

        "length", sol::property([](BinaryView& bv) -> uint64_t {
            return bv.GetLength();
        }),

        "file", sol::property([](BinaryView& bv) -> std::string {
            Ref<FileMetadata> file = bv.GetFile();
            return file ? file->GetFilename() : "<unknown>";
        }),
        // Alias for file
        "filename", sol::property([](BinaryView& bv) -> std::string {
            Ref<FileMetadata> file = bv.GetFile();
            return file ? file->GetFilename() : "<unknown>";
        }),

        "arch", sol::property([](BinaryView& bv) -> std::string {
            Ref<Architecture> arch = bv.GetDefaultArchitecture();
            return arch ? arch->GetName() : "<unknown>";
        }),

        "entry_point", sol::property([](BinaryView& bv) -> HexAddress {
            return HexAddress(bv.GetEntryPoint());
        }),

        // Collection methods - use method syntax: bv:functions(), bv:sections(), etc.
        // Note: sol::property with sol::this_state causes crashes, so these are methods
        "functions", [](sol::this_state ts, BinaryView& bv) -> sol::table {
            sol::state_view lua(ts);
            std::vector<Ref<Function>> funcs = bv.GetAnalysisFunctionList();
            sol::table result = lua.create_table();
            for (size_t i = 0; i < funcs.size(); i++) result[i + 1] = funcs[i];
            return result;
        },

        "sections", [](sol::this_state ts, BinaryView& bv) -> sol::table {
            sol::state_view lua(ts);
            std::vector<Ref<Section>> sects = bv.GetSections();
            sol::table result = lua.create_table();
            for (size_t i = 0; i < sects.size(); i++) result[i + 1] = sects[i];
            return result;
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
            auto addr = GetAddressFromObject(addr_obj);
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
            auto addr = GetAddressFromObject(addr_obj);
            if (!addr) return std::nullopt;
            uint64_t nextAddr = bv.GetNextFunctionStartAfterAddress(*addr);
            if (nextAddr == 0 || nextAddr <= *addr) return std::nullopt;
            return HexAddress(nextAddr);
        },

        "get_previous_function_start_before", [](BinaryView& bv, sol::object addr_obj)
            -> std::optional<HexAddress> {
            auto addr = GetAddressFromObject(addr_obj);
            if (!addr) return std::nullopt;
            uint64_t prevAddr = bv.GetPreviousFunctionStartBeforeAddress(*addr);
            if (prevAddr == 0 || prevAddr >= *addr) return std::nullopt;
            return HexAddress(prevAddr);
        },

        "get_next_basic_block_start_after", [](BinaryView& bv, sol::object addr_obj)
            -> std::optional<HexAddress> {
            auto addr = GetAddressFromObject(addr_obj);
            if (!addr) return std::nullopt;
            uint64_t nextAddr = bv.GetNextBasicBlockStartAfterAddress(*addr);
            if (nextAddr == 0 || nextAddr <= *addr) return std::nullopt;
            return HexAddress(nextAddr);
        },

        "get_previous_basic_block_start_before", [](BinaryView& bv, sol::object addr_obj)
            -> std::optional<HexAddress> {
            auto addr = GetAddressFromObject(addr_obj);
            if (!addr) return std::nullopt;
            uint64_t prevAddr = bv.GetPreviousBasicBlockStartBeforeAddress(*addr);
            if (prevAddr == 0 || prevAddr >= *addr) return std::nullopt;
            return HexAddress(prevAddr);
        },

        "get_next_data_after", [](BinaryView& bv, sol::object addr_obj)
            -> std::optional<HexAddress> {
            auto addr = GetAddressFromObject(addr_obj);
            if (!addr) return std::nullopt;
            uint64_t nextAddr = bv.GetNextDataAfterAddress(*addr);
            if (nextAddr == 0 || nextAddr <= *addr) return std::nullopt;
            return HexAddress(nextAddr);
        },

        "get_previous_data_before", [](BinaryView& bv, sol::object addr_obj)
            -> std::optional<HexAddress> {
            auto addr = GetAddressFromObject(addr_obj);
            if (!addr) return std::nullopt;
            uint64_t prevAddr = bv.GetPreviousDataBeforeAddress(*addr);
            if (prevAddr == 0 || prevAddr >= *addr) return std::nullopt;
            return HexAddress(prevAddr);
        },

        // Advanced lookup functions - all accept HexAddress or uint64_t
        "get_functions_at", [](sol::this_state ts, BinaryView& bv, sol::object addr_obj)
            -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto addr = GetAddressFromObject(addr_obj);
            if (!addr) return result;
            std::vector<Ref<Function>> funcs = bv.GetAnalysisFunctionsForAddress(*addr);
            for (size_t i = 0; i < funcs.size(); i++) {
                result[i + 1] = funcs[i];
            }
            return result;
        },

        "get_functions_containing", [](sol::this_state ts, BinaryView& bv, sol::object addr_obj)
            -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto addr = GetAddressFromObject(addr_obj);
            if (!addr) return result;
            std::vector<Ref<Function>> funcs = bv.GetAnalysisFunctionsContainingAddress(*addr);
            for (size_t i = 0; i < funcs.size(); i++) {
                result[i + 1] = funcs[i];
            }
            return result;
        },

        "get_basic_blocks_starting_at", [](sol::this_state ts, BinaryView& bv, sol::object addr_obj)
            -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto addr = GetAddressFromObject(addr_obj);
            if (!addr) return result;
            std::vector<Ref<BasicBlock>> blocks = bv.GetBasicBlocksStartingAtAddress(*addr);
            for (size_t i = 0; i < blocks.size(); i++) {
                result[i + 1] = blocks[i];
            }
            return result;
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
            auto start = GetAddressFromObject(start_obj);
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

            auto start = GetAddressFromObject(start_obj);
            auto end = GetAddressFromObject(end_obj);
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
            auto start = GetAddressFromObject(start_obj);
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

            auto start = GetAddressFromObject(start_obj);
            auto end = GetAddressFromObject(end_obj);
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
            auto start = GetAddressFromObject(start_obj);
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
            sol::state_view lua(ts);
            std::vector<ReferenceSource> refs = bv.GetCodeReferences(addr);
            sol::table result = lua.create_table();
            for (size_t i = 0; i < refs.size(); i++) {
                sol::table entry = lua.create_table();
                entry["addr"] = HexAddress(refs[i].addr);
                if (refs[i].func) entry["func"] = refs[i].func;
                result[i + 1] = entry;
            }
            return result;
        },

        "get_data_refs", [](sol::this_state ts, BinaryView& bv, uint64_t addr) -> sol::table {
            sol::state_view lua(ts);
            std::vector<uint64_t> refs = bv.GetDataReferences(addr);
            sol::table result = lua.create_table();
            for (size_t i = 0; i < refs.size(); i++) result[i + 1] = HexAddress(refs[i]);
            return result;
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
            sol::state_view lua(ts);
            std::vector<ReferenceSource> refs = bv.GetCallers(addr);
            sol::table result = lua.create_table();
            for (size_t i = 0; i < refs.size(); i++) {
                sol::table entry = lua.create_table();
                entry["address"] = HexAddress(refs[i].addr);
                if (refs[i].func) entry["func"] = refs[i].func;
                if (refs[i].arch) entry["arch"] = refs[i].arch->GetName();
                result[i + 1] = entry;
            }
            return result;
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
            sol::state_view lua(ts);
            std::vector<Ref<TagType>> types = bv.GetTagTypes();
            sol::table result = lua.create_table();
            for (size_t i = 0; i < types.size(); i++) {
                result[i + 1] = types[i];
            }
            return result;
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
            auto addr = GetAddressFromObject(addr_obj);
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
            auto addr = GetAddressFromObject(addr_obj);
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
            auto addr = GetAddressFromObject(addr_obj);
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
            auto addr = GetAddressFromObject(addr_obj);
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
            auto start = GetAddressFromObject(start_obj);
            auto end = GetAddressFromObject(end_obj);
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

            // Convert state to string
            std::string stateStr;
            switch (progress.state) {
                case InitialState: stateStr = "initial"; break;
                case HoldState: stateStr = "hold"; break;
                case IdleState: stateStr = "idle"; break;
                case DiscoveryState: stateStr = "discovery"; break;
                case DisassembleState: stateStr = "disassemble"; break;
                case AnalyzeState: stateStr = "analyze"; break;
                case ExtendedAnalyzeState: stateStr = "extended_analyze"; break;
                default: stateStr = "unknown"; break;
            }

            result["state"] = stateStr;
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
            BNMetadata* md = nullptr;

            if (value.is<bool>()) {
                md = BNCreateMetadataBooleanData(value.as<bool>());
            } else if (value.is<std::string>()) {
                md = BNCreateMetadataStringData(value.as<std::string>().c_str());
            } else if (value.get_type() == sol::type::number) {
                // Handle all numbers - check if integer or float
                double d = value.as<double>();
                // Check if it's actually a whole number (no fractional part)
                if (d == std::floor(d) && d >= INT64_MIN && d <= INT64_MAX) {
                    md = BNCreateMetadataSignedIntegerData(static_cast<int64_t>(d));
                } else {
                    md = BNCreateMetadataDoubleData(d);
                }
            } else if (value.is<sol::table>()) {
                // Check if it's an array or a key-value table
                sol::table tbl = value.as<sol::table>();
                bool isArray = true;
                size_t expectedIdx = 1;
                for (auto& kv : tbl) {
                    if (!kv.first.is<size_t>() || kv.first.as<size_t>() != expectedIdx) {
                        isArray = false;
                        break;
                    }
                    expectedIdx++;
                }

                if (isArray && expectedIdx > 1) {
                    // Array of strings (most common case)
                    std::vector<const char*> strValues;
                    std::vector<std::string> strStorage;
                    bool allStrings = true;

                    for (auto& kv : tbl) {
                        if (kv.second.is<std::string>()) {
                            strStorage.push_back(kv.second.as<std::string>());
                        } else {
                            allStrings = false;
                            break;
                        }
                    }

                    if (allStrings && !strStorage.empty()) {
                        for (const auto& s : strStorage) {
                            strValues.push_back(s.c_str());
                        }
                        md = BNCreateMetadataStringListData(strValues.data(), strValues.size());
                    }
                } else {
                    // Key-value store
                    std::vector<const char*> keys;
                    std::vector<BNMetadata*> values;
                    std::vector<std::string> keyStorage;

                    for (auto& kv : tbl) {
                        if (kv.first.is<std::string>()) {
                            keyStorage.push_back(kv.first.as<std::string>());

                            BNMetadata* val = nullptr;
                            if (kv.second.is<bool>()) {
                                val = BNCreateMetadataBooleanData(kv.second.as<bool>());
                            } else if (kv.second.is<std::string>()) {
                                val = BNCreateMetadataStringData(
                                    kv.second.as<std::string>().c_str());
                            } else if (kv.second.is<int64_t>()) {
                                val = BNCreateMetadataSignedIntegerData(kv.second.as<int64_t>());
                            } else if (kv.second.is<double>()) {
                                val = BNCreateMetadataDoubleData(kv.second.as<double>());
                            }

                            if (val) {
                                values.push_back(val);
                            }
                        }
                    }

                    if (!keyStorage.empty() && keyStorage.size() == values.size()) {
                        for (const auto& k : keyStorage) {
                            keys.push_back(k.c_str());
                        }
                        md = BNCreateMetadataValueStore(keys.data(), values.data(), keys.size());
                    }

                    // Free intermediate metadata objects
                    for (auto* v : values) {
                        BNFreeMetadata(v);
                    }
                }
            }

            if (md) {
                BNBinaryViewStoreMetadata(BinaryView::GetObject(&bv), key.c_str(), md,
                                         isAuto.value_or(false));
                BNFreeMetadata(md);
            }
        },

        // Query metadata
        "query_metadata", [](sol::this_state ts, BinaryView& bv,
                            const std::string& key) -> sol::object {
            sol::state_view lua(ts);

            BNMetadata* md = BNBinaryViewQueryMetadata(BinaryView::GetObject(&bv), key.c_str());
            if (!md) {
                return sol::make_object(lua, sol::nil);
            }

            sol::object result = sol::make_object(lua, sol::nil);
            BNMetadataType type = BNMetadataGetType(md);

            switch (type) {
                case BooleanDataType:
                    result = sol::make_object(lua, BNMetadataGetBoolean(md));
                    break;
                case StringDataType: {
                    char* str = BNMetadataGetString(md);
                    if (str) {
                        result = sol::make_object(lua, std::string(str));
                        BNFreeString(str);
                    }
                    break;
                }
                case UnsignedIntegerDataType:
                    result = sol::make_object(lua, BNMetadataGetUnsignedInteger(md));
                    break;
                case SignedIntegerDataType:
                    result = sol::make_object(lua, BNMetadataGetSignedInteger(md));
                    break;
                case DoubleDataType:
                    result = sol::make_object(lua, BNMetadataGetDouble(md));
                    break;
                case KeyValueDataType: {
                    BNMetadataValueStore* store = BNMetadataGetValueStore(md);
                    if (store) {
                        sol::table tbl = lua.create_table();
                        for (size_t i = 0; i < store->size; i++) {
                            BNMetadataType vtype = BNMetadataGetType(store->values[i]);
                            if (vtype == StringDataType) {
                                char* str = BNMetadataGetString(store->values[i]);
                                if (str) {
                                    tbl[store->keys[i]] = std::string(str);
                                    BNFreeString(str);
                                }
                            } else if (vtype == BooleanDataType) {
                                tbl[store->keys[i]] = BNMetadataGetBoolean(store->values[i]);
                            } else if (vtype == SignedIntegerDataType) {
                                tbl[store->keys[i]] = BNMetadataGetSignedInteger(store->values[i]);
                            } else if (vtype == UnsignedIntegerDataType) {
                                tbl[store->keys[i]] = BNMetadataGetUnsignedInteger(store->values[i]);
                            } else if (vtype == DoubleDataType) {
                                tbl[store->keys[i]] = BNMetadataGetDouble(store->values[i]);
                            }
                        }
                        result = sol::make_object(lua, tbl);
                        BNFreeMetadataValueStore(store);
                    }
                    break;
                }
                default:
                    // For arrays and other types, return as JSON string
                    char* json = BNMetadataGetJsonString(md);
                    if (json) {
                        result = sol::make_object(lua, std::string(json));
                        BNFreeString(json);
                    }
                    break;
            }

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
