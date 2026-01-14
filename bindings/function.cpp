// Sol2 Function bindings for binja-lua

#include "common.h"
#include <set>
#include <cmath>

namespace BinjaLua {

void RegisterFunctionBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering Function bindings");

    lua.new_usertype<Function>(FUNCTION_METATABLE,
        sol::no_constructor,

        // Properties - simple getters returning HexAddress for consistency
        "start_addr", sol::property([](Function& f) -> HexAddress {
            return HexAddress(f.GetStart());
        }),
        // Alias for start_addr
        "start", sol::property([](Function& f) -> HexAddress {
            return HexAddress(f.GetStart());
        }),

        "end_addr", sol::property([](Function& f) -> HexAddress {
            return HexAddress(f.GetHighestAddress());
        }),
        // Alias for end_addr
        "end", sol::property([](Function& f) -> HexAddress {
            return HexAddress(f.GetHighestAddress());
        }),

        "size", sol::property([](Function& f) -> uint64_t {
            return f.GetHighestAddress() - f.GetStart();
        }),

        "name", sol::property([](Function& f) -> std::string {
            Ref<Symbol> sym = f.GetSymbol();
            return sym ? sym->GetShortName() : "<unnamed>";
        }),

        "arch", sol::property([](Function& f) -> std::string {
            Ref<Architecture> arch = f.GetArchitecture();
            return arch ? arch->GetName() : "<unknown>";
        }),

        "comment", sol::property([](Function& f) -> std::string {
            return f.GetComment();
        }),

        // Symbol accessor
        "symbol", sol::property([](Function& f) -> Ref<Symbol> {
            return f.GetSymbol();
        }),

        // BinaryView accessor
        "view", sol::property([](Function& f) -> Ref<BinaryView> {
            return f.GetView();
        }),

        // Boolean analysis properties
        "auto_discovered", sol::property([](Function& f) -> bool {
            return f.WasAutomaticallyDiscovered();
        }),

        "has_user_annotations", sol::property([](Function& f) -> bool {
            return f.HasUserAnnotations();
        }),

        "is_pure", sol::property([](Function& f) -> bool {
            return f.IsPure().GetValue();
        }),

        "has_explicitly_defined_type", sol::property([](Function& f) -> bool {
            return f.HasExplicitlyDefinedType();
        }),

        "has_user_type", sol::property([](Function& f) -> bool {
            return f.HasUserType();
        }),

        "has_unresolved_indirect_branches", sol::property([](Function& f) -> bool {
            return f.HasUnresolvedIndirectBranches();
        }),

        "analysis_skipped", sol::property([](Function& f) -> bool {
            return f.IsAnalysisSkipped();
        }),

        "too_large", sol::property([](Function& f) -> bool {
            return f.IsFunctionTooLarge();
        }),

        "needs_update", sol::property([](Function& f) -> bool {
            return f.NeedsUpdate();
        }),

        "analysis_skip_reason", sol::property([](Function& f) -> std::string {
            BNAnalysisSkipReason reason = f.GetAnalysisSkipReason();
            switch (reason) {
                case NoSkipReason: return "none";
                case AlwaysSkipReason: return "always";
                case ExceedFunctionSizeSkipReason: return "exceed_size";
                case ExceedFunctionAnalysisTimeSkipReason: return "exceed_time";
                case ExceedFunctionUpdateCountSkipReason: return "exceed_updates";
                case NewAutoFunctionAnalysisSuppressedReason: return "new_auto_suppressed";
                case BasicAnalysisSkipReason: return "basic_analysis";
                case IntermediateAnalysisSkipReason: return "intermediate_analysis";
                case AnalysisPipelineSuspendedReason: return "pipeline_suspended";
                default: return "unknown";
            }
        }),

        "can_return", sol::property([](Function& f) -> bool {
            return f.CanReturn().GetValue();
        }),

        // Alias for auto_discovered for Python API compatibility
        "auto", sol::property([](Function& f) -> bool {
            return f.WasAutomaticallyDiscovered();
        }),

        // is_exported: true if function symbol has global or weak binding
        "is_exported", sol::property([](Function& f) -> bool {
            Ref<Symbol> sym = f.GetSymbol();
            if (!sym) return false;
            BNSymbolBinding binding = sym->GetBinding();
            return binding == GlobalBinding || binding == WeakBinding;
        }),

        // is_inlined_during_analysis: whether function is inlined during analysis
        "is_inlined_during_analysis", sol::property([](Function& f) -> bool {
            return f.IsInlinedDuringAnalysis().GetValue();
        }),

        // is_thunk: true if function is a single-block tailcall thunk
        "is_thunk", sol::property([](Function& f) -> bool {
            Ref<LowLevelILFunction> llil = f.GetLowLevelIL();
            if (!llil) return false;
            auto blocks = llil->GetBasicBlocks();
            if (blocks.size() != 1) return false;
            // Check if the single block ends with a tailcall
            Ref<BasicBlock> block = blocks[0];
            size_t startIdx = block->GetStart();
            size_t endIdx = block->GetEnd();
            if (endIdx <= startIdx) return false;
            // Get the last instruction via BN API
            size_t lastIdx = endIdx - 1;
            BNLowLevelILInstruction instr = BNGetLowLevelILByIndex(
                llil->GetObject(), lastIdx);
            return instr.operation == LLIL_TAILCALL ||
                   instr.operation == LLIL_TAILCALL_SSA;
        }),

        // Collection methods - use method syntax: func:basic_blocks(), func:callers(), etc.
        // Note: sol::property with sol::this_state causes crashes, so these are methods
        "basic_blocks", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            std::vector<Ref<BasicBlock>> blocks = f.GetBasicBlocks();
            sol::table result = lua.create_table();
            for (size_t i = 0; i < blocks.size(); i++) {
                result[i + 1] = blocks[i];
            }
            return result;
        },

        "calls", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            std::vector<ReferenceSource> call_sites = f.GetCallSites();
            std::vector<Ref<Function>> called;
            Ref<BinaryView> view = f.GetView();

            for (const auto& site : call_sites) {
                for (uint64_t addr : view->GetCallees(site)) {
                    Ref<Function> fn = view->GetAnalysisFunction(f.GetPlatform(), addr);
                    if (fn) called.push_back(fn);
                }
            }

            sol::table result = lua.create_table();
            for (size_t i = 0; i < called.size(); i++) {
                result[i + 1] = called[i];
            }
            return result;
        },

        "callers", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            Ref<BinaryView> view = f.GetView();
            std::vector<ReferenceSource> refs = view->GetCallers(f.GetStart());

            sol::table result = lua.create_table();
            int idx = 1;
            for (const auto& ref : refs) {
                if (ref.func) result[idx++] = ref.func;
            }
            return result;
        },

        // Additional cross-reference methods
        "call_sites", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            std::vector<ReferenceSource> sites = f.GetCallSites();
            sol::table result = lua.create_table();
            for (size_t i = 0; i < sites.size(); i++) {
                sol::table entry = lua.create_table();
                entry["address"] = HexAddress(sites[i].addr);
                if (sites[i].func) entry["func"] = sites[i].func;
                if (sites[i].arch) entry["arch"] = sites[i].arch->GetName();
                result[i + 1] = entry;
            }
            return result;
        },

        "callees", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            std::vector<ReferenceSource> call_sites = f.GetCallSites();
            Ref<BinaryView> view = f.GetView();
            std::vector<Ref<Function>> funcs;
            std::set<uint64_t> seenAddrs;

            for (const auto& site : call_sites) {
                for (uint64_t addr : view->GetCallees(site)) {
                    if (seenAddrs.find(addr) == seenAddrs.end()) {
                        Ref<Function> fn = view->GetAnalysisFunction(f.GetPlatform(), addr);
                        if (fn) {
                            funcs.push_back(fn);
                            seenAddrs.insert(addr);
                        }
                    }
                }
            }

            sol::table result = lua.create_table();
            for (size_t i = 0; i < funcs.size(); i++) {
                result[i + 1] = funcs[i];
            }
            return result;
        },

        "callee_addresses", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            std::vector<ReferenceSource> call_sites = f.GetCallSites();
            Ref<BinaryView> view = f.GetView();
            std::set<uint64_t> uniqueAddrs;

            for (const auto& site : call_sites) {
                for (uint64_t addr : view->GetCallees(site)) {
                    uniqueAddrs.insert(addr);
                }
            }

            sol::table result = lua.create_table();
            int idx = 1;
            for (uint64_t addr : uniqueAddrs) {
                result[idx++] = HexAddress(addr);
            }
            return result;
        },

        "caller_sites", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            Ref<BinaryView> view = f.GetView();
            std::vector<ReferenceSource> refs = view->GetCallers(f.GetStart());
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

        "variables", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            Ref<Function> func = &f;
            std::map<Variable, VariableNameAndType> allVars = f.GetVariables();

            sol::table result = lua.create_table();
            int idx = 1;
            for (const auto& pair : allVars) {
                BNVariable bnVar = {pair.first.type, pair.first.index, pair.first.storage};
                result[idx++] = VariableWrapper(bnVar, func);
            }
            return result;
        },

        "parameter_vars", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            Ref<Function> func = &f;
            Confidence<std::vector<Variable>> paramVars = f.GetParameterVariables();

            sol::table result = lua.create_table();
            int idx = 1;
            for (const auto& var : paramVars.GetValue()) {
                BNVariable bnVar = {var.type, var.index, var.storage};
                result[idx++] = VariableWrapper(bnVar, func);
            }
            return result;
        },

        "stack_adjustment", sol::property([](Function& f) -> int64_t {
            return f.GetStackAdjustment().GetValue();
        }),

        "clobbered_regs", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            Ref<Architecture> arch = f.GetArchitecture();
            if (!arch) return result;

            Confidence<std::set<uint32_t>> regs = f.GetClobberedRegisters();
            int idx = 1;
            for (uint32_t reg : regs.GetValue()) {
                result[idx++] = arch->GetRegisterName(reg);
            }
            return result;
        },

        "get_variable_by_name", [](Function& f, const std::string& name)
            -> std::optional<VariableWrapper> {
            Ref<Function> func = &f;
            std::map<Variable, VariableNameAndType> allVars = f.GetVariables();
            for (const auto& pair : allVars) {
                if (pair.second.name == name) {
                    BNVariable bnVar = {pair.first.type, pair.first.index, pair.first.storage};
                    return VariableWrapper(bnVar, func);
                }
            }
            return std::nullopt;
        },

        "create_user_var", [](Function& f, int64_t storage, const std::string& typeStr,
                              const std::string& name) -> bool {
            Ref<BinaryView> bv = f.GetView();
            if (!bv) return false;

            QualifiedNameAndType result;
            std::string errors;
            if (!bv->ParseTypeString(typeStr, result, errors)) {
                return false;
            }

            Variable var;
            var.type = StackVariableSourceType;
            var.index = 0;
            var.storage = storage;

            Confidence<Ref<Type>> typeConf(result.type, 255);
            f.CreateUserVariable(var, typeConf, name);
            return true;
        },

        "delete_user_var", [](Function& f, VariableWrapper& varWrapper) {
            Variable var;
            var.type = static_cast<BNVariableSourceType>(varWrapper.bnVar.type);
            var.index = varWrapper.bnVar.index;
            var.storage = varWrapper.bnVar.storage;
            f.DeleteUserVariable(var);
        },

        // Comment methods
        "comment_at_address", [](Function& f, uint64_t addr) -> std::string {
            return f.GetCommentForAddress(addr);
        },

        "set_comment", [](Function& f, const std::string& comment) -> bool {
            f.SetComment(comment);
            return true;
        },

        "set_comment_at_address", [](Function& f, uint64_t addr, const std::string& comment) -> bool {
            f.SetCommentForAddress(addr, comment);
            return true;
        },

        // Function modification
        "set_name", [](Function& f, const std::string& name) -> bool {
            Ref<BinaryView> bv = f.GetView();
            uint64_t addr = f.GetStart();
            Ref<Symbol> existing = bv->GetSymbolByAddress(addr);
            if (existing) bv->UndefineUserSymbol(existing);
            Ref<Symbol> newSym = new Symbol(FunctionSymbol, name, addr);
            bv->DefineUserSymbol(newSym);
            return true;
        },

        "set_return_type", [](Function& f, const std::string& typeStr,
                              sol::optional<bool> reanalyze) -> bool {
            Ref<BinaryView> bv = f.GetView();

            // Parse the new return type
            QualifiedNameAndType result;
            std::string errors;
            if (!bv->ParseTypeString(typeStr, result, errors)) {
                return false;
            }

            // Get current function type to preserve parameters
            Ref<Type> currentType = f.GetType();
            if (!currentType) {
                return false;
            }

            // Build new function type with new return type
            std::vector<FunctionParameter> params = currentType->GetParameters();
            Confidence<bool> varArgs = currentType->HasVariableArguments();
            Confidence<Ref<CallingConvention>> cc = f.GetCallingConvention();

            Ref<Type> newFuncType = Type::FunctionType(
                Confidence<Ref<Type>>(result.type, 255),
                cc,
                params,
                varArgs
            );

            // Set as user type for persistence
            f.SetUserType(newFuncType);

            // Auto-reanalyze by default
            if (!reanalyze.has_value() || reanalyze.value()) {
                f.Reanalyze(UserFunctionUpdate);
            }
            return true;
        },

        "set_parameter_type", [](Function& f, size_t index, const std::string& typeStr,
                                 sol::optional<bool> reanalyze) -> bool {
            Ref<BinaryView> bv = f.GetView();

            // Parse the new parameter type
            QualifiedNameAndType result;
            std::string errors;
            if (!bv->ParseTypeString(typeStr, result, errors)) {
                return false;
            }

            // Get current function type
            Ref<Type> currentType = f.GetType();
            if (!currentType) {
                return false;
            }

            std::vector<FunctionParameter> params = currentType->GetParameters();
            if (index >= params.size()) {
                return false;  // Index out of bounds
            }

            // Update the parameter type
            params[index].type = Confidence<Ref<Type>>(result.type, 255);

            // Rebuild function type
            Confidence<Ref<Type>> retType = f.GetReturnType();
            Confidence<Ref<CallingConvention>> cc = f.GetCallingConvention();
            Confidence<bool> varArgs = currentType->HasVariableArguments();

            Ref<Type> newFuncType = Type::FunctionType(retType, cc, params, varArgs);
            f.SetUserType(newFuncType);

            if (!reanalyze.has_value() || reanalyze.value()) {
                f.Reanalyze(UserFunctionUpdate);
            }
            return true;
        },

        "set_parameter_name", [](Function& f, size_t index, const std::string& name,
                                 sol::optional<bool> reanalyze) -> bool {
            // Get current function type
            Ref<Type> currentType = f.GetType();
            if (!currentType) {
                return false;
            }

            std::vector<FunctionParameter> params = currentType->GetParameters();
            if (index >= params.size()) {
                return false;  // Index out of bounds
            }

            // Update the parameter name
            params[index].name = name;

            // Rebuild function type
            Confidence<Ref<Type>> retType = f.GetReturnType();
            Confidence<Ref<CallingConvention>> cc = f.GetCallingConvention();
            Confidence<bool> varArgs = currentType->HasVariableArguments();

            Ref<Type> newFuncType = Type::FunctionType(retType, cc, params, varArgs);
            f.SetUserType(newFuncType);

            if (!reanalyze.has_value() || reanalyze.value()) {
                f.Reanalyze(UserFunctionUpdate);
            }
            return true;
        },

        "add_parameter", [](Function& f, const std::string& typeStr,
                           sol::optional<std::string> name,
                           sol::optional<bool> reanalyze) -> bool {
            Ref<BinaryView> bv = f.GetView();

            // Parse the parameter type
            QualifiedNameAndType result;
            std::string errors;
            if (!bv->ParseTypeString(typeStr, result, errors)) {
                return false;
            }

            // Get current function type
            Ref<Type> currentType = f.GetType();
            if (!currentType) {
                return false;
            }

            std::vector<FunctionParameter> params = currentType->GetParameters();

            // Add new parameter
            FunctionParameter newParam;
            newParam.name = name.value_or("");
            newParam.type = Confidence<Ref<Type>>(result.type, 255);
            params.push_back(newParam);

            // Rebuild function type
            Confidence<Ref<Type>> retType = f.GetReturnType();
            Confidence<Ref<CallingConvention>> cc = f.GetCallingConvention();
            Confidence<bool> varArgs = currentType->HasVariableArguments();

            Ref<Type> newFuncType = Type::FunctionType(retType, cc, params, varArgs);
            f.SetUserType(newFuncType);

            if (!reanalyze.has_value() || reanalyze.value()) {
                f.Reanalyze(UserFunctionUpdate);
            }
            return true;
        },

        "remove_parameter", [](Function& f, size_t index,
                              sol::optional<bool> reanalyze) -> bool {
            // Get current function type
            Ref<Type> currentType = f.GetType();
            if (!currentType) {
                return false;
            }

            std::vector<FunctionParameter> params = currentType->GetParameters();
            if (index >= params.size()) {
                return false;  // Index out of bounds
            }

            // Remove the parameter
            params.erase(params.begin() + index);

            // Rebuild function type
            Confidence<Ref<Type>> retType = f.GetReturnType();
            Confidence<Ref<CallingConvention>> cc = f.GetCallingConvention();
            Confidence<bool> varArgs = currentType->HasVariableArguments();

            Ref<Type> newFuncType = Type::FunctionType(retType, cc, params, varArgs);
            f.SetUserType(newFuncType);

            if (!reanalyze.has_value() || reanalyze.value()) {
                f.Reanalyze(UserFunctionUpdate);
            }
            return true;
        },

        "set_calling_convention", [](Function& f, const std::string& ccName,
                                     sol::optional<bool> reanalyze) -> bool {
            Ref<BinaryView> bv = f.GetView();
            Ref<Platform> platform = f.GetPlatform();
            if (!platform) return false;

            // Find the calling convention
            Ref<CallingConvention> newCC;
            std::vector<Ref<CallingConvention>> ccs = platform->GetCallingConventions();
            for (const auto& cc : ccs) {
                if (cc->GetName() == ccName) {
                    newCC = cc;
                    break;
                }
            }
            if (!newCC) return false;

            // Get current function type to preserve other properties
            Ref<Type> currentType = f.GetType();
            if (!currentType) {
                return false;
            }

            // Build new function type with new calling convention
            Confidence<Ref<Type>> retType = f.GetReturnType();
            std::vector<FunctionParameter> params = currentType->GetParameters();
            Confidence<bool> varArgs = currentType->HasVariableArguments();

            Ref<Type> newFuncType = Type::FunctionType(
                retType,
                Confidence<Ref<CallingConvention>>(newCC, 255),
                params,
                varArgs
            );

            // Set as user type for persistence
            f.SetUserType(newFuncType);

            // Auto-reanalyze by default
            if (!reanalyze.has_value() || reanalyze.value()) {
                f.Reanalyze(UserFunctionUpdate);
            }
            return true;
        },

        "set_can_return", [](Function& f, bool canReturn, sol::optional<bool> reanalyze) {
            // Use SetCanReturn directly - it persists as a user override
            Confidence<bool> conf(canReturn, 255);
            f.SetCanReturn(conf);

            // Auto-reanalyze by default
            if (!reanalyze.has_value() || reanalyze.value()) {
                f.Reanalyze(UserFunctionUpdate);
            }
        },

        "set_has_variable_arguments", [](Function& f, bool hasVarArgs,
                                         sol::optional<bool> reanalyze) {
            // Get current function type to preserve other properties
            Ref<Type> currentType = f.GetType();
            if (!currentType) {
                Confidence<bool> conf(hasVarArgs, 255);
                f.SetHasVariableArguments(conf);
            } else {
                // Build new function type with new varargs setting
                Confidence<Ref<Type>> retType = f.GetReturnType();
                Confidence<Ref<CallingConvention>> cc = f.GetCallingConvention();
                std::vector<FunctionParameter> params = currentType->GetParameters();

                Ref<Type> newFuncType = Type::FunctionType(
                    retType,
                    cc,
                    params,
                    Confidence<bool>(hasVarArgs, 255)
                );

                f.SetUserType(newFuncType);
            }

            // Auto-reanalyze by default
            if (!reanalyze.has_value() || reanalyze.value()) {
                f.Reanalyze(UserFunctionUpdate);
            }
        },

        "set_is_pure", [](Function& f, bool isPure, sol::optional<bool> reanalyze) {
            Confidence<bool> conf(isPure, 255);
            f.SetPure(conf);

            // Auto-reanalyze by default
            if (!reanalyze.has_value() || reanalyze.value()) {
                f.Reanalyze(UserFunctionUpdate);
            }
        },

        "set_analysis_skipped", [](Function& f, bool skip) {
            if (skip) {
                f.SetAnalysisSkipOverride(AlwaysSkipFunctionAnalysis);
            } else {
                f.SetAnalysisSkipOverride(NeverSkipFunctionAnalysis);
            }
        },

        "set_user_inlined", [](Function& f, bool inlined, sol::optional<bool> reanalyze) {
            Confidence<bool> conf(inlined, 255);
            f.SetUserInlinedDuringAnalysis(conf);

            // Auto-reanalyze by default
            if (!reanalyze.has_value() || reanalyze.value()) {
                f.Reanalyze(UserFunctionUpdate);
            }
        },

        // Reanalysis - triggers function reanalysis to update UI
        "reanalyze", [](Function& f) {
            f.Reanalyze(UserFunctionUpdate);
        },

        "mark_updates_required", [](Function& f) {
            f.MarkUpdatesRequired(UserFunctionUpdate);
        },

        // IL accessors
        "get_llil", [](Function& f) -> Ref<LowLevelILFunction> {
            return f.GetLowLevelIL();
        },

        "get_mlil", [](Function& f) -> Ref<MediumLevelILFunction> {
            return f.GetMediumLevelIL();
        },

        "get_hlil", [](Function& f) -> Ref<HighLevelILFunction> {
            return f.GetHighLevelIL();
        },

        // Type information - use method syntax: func:type()
        "type", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            Ref<Type> funcType = f.GetType();
            if (!funcType) return result;

            Confidence<Ref<Type>> retType = f.GetReturnType();
            if (retType.GetValue()) result["return_type"] = retType->GetString();

            sol::table params = lua.create_table();
            std::vector<FunctionParameter> funcParams = funcType->GetParameters();
            for (size_t i = 0; i < funcParams.size(); i++) {
                sol::table param = lua.create_table();
                if (!funcParams[i].name.empty()) param["name"] = funcParams[i].name;
                if (funcParams[i].type.GetValue()) param["type"] = funcParams[i].type->GetString();
                params[i + 1] = param;
            }
            result["parameters"] = params;

            Confidence<Ref<CallingConvention>> cc = f.GetCallingConvention();
            if (cc.GetValue()) result["calling_convention"] = cc->GetName();
            result["has_variable_args"] = funcType->HasVariableArguments();
            return result;
        },

        // Disassembly - use method syntax: func:disassembly()
        "disassembly", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            std::vector<Ref<BasicBlock>> blocks = f.GetBasicBlocks();
            Ref<BinaryView> view = f.GetView();
            Ref<Architecture> arch = f.GetArchitecture();

            std::vector<InstructionWrapper> instrs;
            for (const auto& block : blocks) {
                auto lines = block->GetDisassemblyText(DisassemblySettings::GetDefaultSettings());
                for (const auto& line : lines) {
                    std::vector<BNInstructionTextToken> tokens;
                    for (const auto& token : line.tokens) tokens.push_back(token.GetAPIObject());
                    instrs.emplace_back(line.addr, tokens, view, arch);
                }
            }
            std::sort(instrs.begin(), instrs.end(),
                [](const InstructionWrapper& a, const InstructionWrapper& b) {
                    return a.address < b.address;
                });

            sol::table result = lua.create_table();
            for (size_t i = 0; i < instrs.size(); i++) result[i + 1] = instrs[i];
            return result;
        },

        // Control flow graph - use method syntax: func:control_flow_graph()
        "control_flow_graph", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            std::vector<Ref<BasicBlock>> blocks = f.GetBasicBlocks();
            sol::table blocksTable = lua.create_table();
            size_t totalEdges = 0;

            for (size_t i = 0; i < blocks.size(); i++) {
                sol::table blockInfo = lua.create_table();
                blockInfo["block"] = blocks[i];
                sol::table outEdges = lua.create_table();
                auto outgoing = blocks[i]->GetOutgoingEdges();
                for (size_t j = 0; j < outgoing.size(); j++) {
                    sol::table edge = lua.create_table();
                    edge["target_index"] = outgoing[j].target->GetIndex();
                    edge["target_addr"] = HexAddress(outgoing[j].target->GetStart());
                    edge["back_edge"] = outgoing[j].backEdge;
                    edge["fall_through"] = outgoing[j].fallThrough;
                    const char* t = "unknown";
                    switch (outgoing[j].type) {
                        case UnconditionalBranch: t = "unconditional"; break;
                        case FalseBranch: t = "false"; break;
                        case TrueBranch: t = "true"; break;
                        case CallDestination: t = "call"; break;
                        case FunctionReturn: t = "return"; break;
                        case IndirectBranch: t = "indirect"; break;
                        default: break;
                    }
                    edge["type"] = t;
                    outEdges[j + 1] = edge;
                }
                blockInfo["outgoing_edges"] = outEdges;
                totalEdges += outgoing.size();
                blocksTable[i + 1] = blockInfo;
            }
            result["blocks"] = blocksTable;
            result["total_blocks"] = blocks.size();
            result["total_edges"] = totalEdges;
            return result;
        },

        // Stack layout - use method syntax: func:stack_layout()
        "stack_layout", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            sol::table varsTable = lua.create_table();
            auto varsMap = f.GetVariables();
            int idx = 1;
            for (const auto& pair : varsMap) {
                if (pair.first.type != StackVariableSourceType) continue;
                sol::table varInfo = lua.create_table();
                varInfo["name"] = f.GetVariableName(pair.first);
                varInfo["offset"] = pair.first.storage;
                Confidence<Ref<Type>> varType = f.GetVariableType(pair.first);
                if (varType.GetValue()) {
                    varInfo["type"] = varType->GetString();
                    varInfo["size"] = varType->GetWidth();
                }
                varInfo["storage_type"] = "stack";
                varsTable[idx++] = varInfo;
            }
            result["variables"] = varsTable;
            result["total_variables"] = idx - 1;
            return result;
        },

        // Stack adjustment - the expected stack pointer change from function entry to exit
        "stack_adjustment", sol::property([](Function& f) -> sol::optional<int64_t> {
            Confidence<int64_t> adj = f.GetStackAdjustment();
            if (adj.GetConfidence() == 0) return sol::nullopt;
            return adj.GetValue();
        }),

        // Clobbered registers - registers that may be modified by this function
        "clobbered_regs", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            Confidence<std::set<uint32_t>> regs = f.GetClobberedRegisters();
            if (regs.GetConfidence() == 0) return result;

            Ref<Architecture> arch = f.GetArchitecture();
            if (!arch) return result;

            int idx = 1;
            for (uint32_t reg : regs.GetValue()) {
                std::string regName = arch->GetRegisterName(reg);
                if (!regName.empty()) {
                    result[idx++] = regName;
                }
            }
            return result;
        },

        // Variable tracking (SSA analysis)
        "merged_vars", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            Ref<Function> func = &f;

            std::map<Variable, std::set<Variable>> merged = f.GetMergedVariables();
            int idx = 1;
            for (const auto& [target, sources] : merged) {
                sol::table entry = lua.create_table();

                // Create target variable wrapper
                BNVariable bnTarget = {target.type, target.index, target.storage};
                entry["target"] = VariableWrapper(bnTarget, func);

                // Create sources table
                sol::table sourcesTable = lua.create_table();
                int srcIdx = 1;
                for (const auto& src : sources) {
                    BNVariable bnSrc = {src.type, src.index, src.storage};
                    sourcesTable[srcIdx++] = VariableWrapper(bnSrc, func);
                }
                entry["sources"] = sourcesTable;

                result[idx++] = entry;
            }
            return result;
        },

        "split_vars", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            Ref<Function> func = &f;

            std::set<Variable> splitVars = f.GetSplitVariables();
            int idx = 1;
            for (const auto& var : splitVars) {
                BNVariable bnVar = {var.type, var.index, var.storage};
                result[idx++] = VariableWrapper(bnVar, func);
            }
            return result;
        },

        "split_variable", [](Function& f, const VariableWrapper& var) {
            Variable v;
            v.type = static_cast<BNVariableSourceType>(var.bnVar.type);
            v.index = var.bnVar.index;
            v.storage = var.bnVar.storage;
            f.SplitVariable(v);
        },

        // IL variable references - get locations where a variable is used
        "get_mlil_var_refs", [](sol::this_state ts, Function& f,
                               const VariableWrapper& varWrapper) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();

            Variable var;
            var.type = static_cast<BNVariableSourceType>(varWrapper.bnVar.type);
            var.index = varWrapper.bnVar.index;
            var.storage = varWrapper.bnVar.storage;

            std::vector<ILReferenceSource> refs = f.GetMediumLevelILVariableReferences(var);
            int idx = 1;
            for (const auto& ref : refs) {
                sol::table entry = lua.create_table();
                entry["addr"] = HexAddress(ref.addr);
                entry["expr_id"] = ref.exprId;
                if (ref.func) entry["func"] = ref.func;
                result[idx++] = entry;
            }
            return result;
        },

        "get_hlil_var_refs", [](sol::this_state ts, Function& f,
                               const VariableWrapper& varWrapper) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();

            Variable var;
            var.type = static_cast<BNVariableSourceType>(varWrapper.bnVar.type);
            var.index = varWrapper.bnVar.index;
            var.storage = varWrapper.bnVar.storage;

            std::vector<ILReferenceSource> refs = f.GetHighLevelILVariableReferences(var);
            int idx = 1;
            for (const auto& ref : refs) {
                sol::table entry = lua.create_table();
                entry["addr"] = HexAddress(ref.addr);
                entry["expr_id"] = ref.exprId;
                if (ref.func) entry["func"] = ref.func;
                result[idx++] = entry;
            }
            return result;
        },

        // Tag operations
        "get_tags", [](sol::this_state ts, Function& f) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            std::vector<TagReference> refs = f.GetAllTagReferences();
            for (size_t i = 0; i < refs.size(); i++) {
                sol::table entry = lua.create_table();
                entry["addr"] = HexAddress(refs[i].addr);
                entry["tag"] = refs[i].tag;
                entry["auto"] = refs[i].autoDefined;
                result[i + 1] = entry;
            }
            return result;
        },

        "get_tags_at", [](sol::this_state ts, Function& f, uint64_t addr) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            Ref<Architecture> arch = f.GetArchitecture();
            std::vector<Ref<Tag>> tags = f.GetAddressTags(arch, addr);
            for (size_t i = 0; i < tags.size(); i++) {
                result[i + 1] = tags[i];
            }
            return result;
        },

        "add_tag", [](Function& f, uint64_t addr, Ref<Tag> tag, sol::optional<bool> user) {
            Ref<Architecture> arch = f.GetArchitecture();
            bool isUser = user.value_or(true);
            if (isUser) {
                f.AddUserAddressTag(arch, addr, tag);
            } else {
                f.AddAutoAddressTag(arch, addr, tag);
            }
        },

        "remove_tag", [](Function& f, uint64_t addr, Ref<Tag> tag, sol::optional<bool> user) {
            Ref<Architecture> arch = f.GetArchitecture();
            bool isUser = user.value_or(true);
            if (isUser) {
                f.RemoveUserAddressTag(arch, addr, tag);
            } else {
                f.RemoveAutoAddressTag(arch, addr, tag);
            }
        },

        "create_user_tag", [](Function& f, uint64_t addr,
                              const std::string& tagTypeName,
                              const std::string& data) -> Ref<Tag> {
            Ref<Architecture> arch = f.GetArchitecture();
            return f.CreateUserAddressTag(arch, addr, tagTypeName, data);
        },

        // ============================================================
        // Metadata System
        // ============================================================

        // Store metadata (supports bool, string, integer, number, table)
        "store_metadata", [](sol::this_state ts, Function& f,
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
                // Key-value store
                sol::table tbl = value.as<sol::table>();
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

                for (auto* v : values) {
                    BNFreeMetadata(v);
                }
            }

            if (md) {
                BNFunctionStoreMetadata(Function::GetObject(&f), key.c_str(), md,
                                       isAuto.value_or(false));
                BNFreeMetadata(md);
            }
        },

        // Query metadata
        "query_metadata", [](sol::this_state ts, Function& f,
                            const std::string& key) -> sol::object {
            sol::state_view lua(ts);

            BNMetadata* md = BNFunctionQueryMetadata(Function::GetObject(&f), key.c_str());
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
        "remove_metadata", [](Function& f, const std::string& key) {
            BNFunctionRemoveMetadata(Function::GetObject(&f), key.c_str());
        },

        // ============================================================
        // Flow Graph Creation
        // ============================================================

        // Create flow graph from function
        "create_graph", [](Function& f, sol::optional<std::string> type) -> Ref<FlowGraph> {
            std::string graphType = type.value_or("normal");

            if (graphType == "llil") {
                Ref<LowLevelILFunction> llil = f.GetLowLevelIL();
                if (llil) return llil->CreateFunctionGraph();
            } else if (graphType == "mlil") {
                Ref<MediumLevelILFunction> mlil = f.GetMediumLevelIL();
                if (mlil) return mlil->CreateFunctionGraph();
            } else if (graphType == "hlil") {
                Ref<HighLevelILFunction> hlil = f.GetHighLevelIL();
                if (hlil) return hlil->CreateFunctionGraph();
            }

            // Default: normal disassembly graph
            return f.CreateFunctionGraph(NormalFunctionGraph, nullptr);
        },

        // Create flow graph and wait for layout to complete
        "create_graph_immediate", [](Function& f, sol::optional<std::string> type)
            -> Ref<FlowGraph> {
            std::string graphType = type.value_or("normal");

            if (graphType == "llil") {
                Ref<LowLevelILFunction> llil = f.GetLowLevelIL();
                if (llil) return llil->CreateFunctionGraphImmediate();
            } else if (graphType == "mlil") {
                Ref<MediumLevelILFunction> mlil = f.GetMediumLevelIL();
                if (mlil) return mlil->CreateFunctionGraphImmediate();
            } else if (graphType == "hlil") {
                Ref<HighLevelILFunction> hlil = f.GetHighLevelIL();
                if (hlil) return hlil->CreateFunctionGraphImmediate();
            }

            // Default: normal disassembly graph (immediate)
            return f.CreateFunctionGraphImmediate(NormalFunctionGraph, nullptr);
        },

        // Comparison
        sol::meta_function::equal_to, [](Function& a, Function& b) -> bool {
            return Function::GetObject(&a) == Function::GetObject(&b);
        },

        // String representation
        sol::meta_function::to_string, [](Function& f) -> std::string {
            Ref<Symbol> sym = f.GetSymbol();
            std::string name = sym ? sym->GetShortName() : "<unnamed>";
            return fmt::format("<Function: {} @ 0x{:x}>", name, f.GetStart());
        }
    );

    if (logger) logger->LogDebug("Function bindings registered");
}

} // namespace BinjaLua
