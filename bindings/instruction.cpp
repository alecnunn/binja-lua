// Sol2 Instruction bindings for binja-lua

#include "common.h"

namespace BinjaLua {

// InstructionWrapper Implementation

InstructionWrapper::InstructionWrapper(uint64_t addr,
                                       const std::vector<BNInstructionTextToken>& inst_tokens,
                                       Ref<BinaryView> bv, Ref<Architecture> architecture)
    : address(addr), tokens(inst_tokens), view(bv), arch(architecture) {
    mnemonic = GetMnemonic();
}

InstructionWrapper::InstructionWrapper(const InstructionWrapper& other)
    : address(other.address), mnemonic(other.mnemonic), tokens(other.tokens),
      view(other.view), arch(other.arch) {
}

InstructionWrapper::~InstructionWrapper() {
}

std::string InstructionWrapper::GetMnemonic() const {
    for (const auto& token : tokens) {
        if (token.type == InstructionToken) {
            return std::string(token.text);
        }
    }
    return "<unknown>";
}

size_t InstructionWrapper::GetLength() const {
    if (view && arch) {
        return view->GetInstructionLength(arch, address);
    }
    return 0;
}

std::vector<uint8_t> InstructionWrapper::GetBytes() const {
    size_t length = GetLength();
    if (length == 0 || !view) {
        return {};
    }

    DataBuffer buffer = view->ReadBuffer(address, length);
    const uint8_t* data = static_cast<const uint8_t*>(buffer.GetData());
    return std::vector<uint8_t>(data, data + length);
}

std::vector<std::string> InstructionWrapper::GetOperands() const {
    std::vector<std::string> operands;
    bool foundMnemonic = false;
    std::string currentOperand;

    for (const auto& token : tokens) {
        if (!foundMnemonic && token.type == InstructionToken) {
            foundMnemonic = true;
            continue;
        }

        if (foundMnemonic) {
            if (token.type == OperandSeparatorToken) {
                if (!currentOperand.empty()) {
                    operands.push_back(currentOperand);
                    currentOperand.clear();
                }
            } else {
                currentOperand += token.text;
            }
        }
    }

    if (!currentOperand.empty()) {
        operands.push_back(currentOperand);
    }

    return operands;
}

std::vector<uint64_t> InstructionWrapper::GetCodeReferences() const {
    std::vector<uint64_t> refs;
    if (!view) {
        return refs;
    }

    for (const auto& token : tokens) {
        if (token.type == PossibleAddressToken || token.type == IntegerToken) {
            if (view->GetAnalysisFunction(view->GetDefaultPlatform(), token.value)) {
                refs.push_back(token.value);
            }
        }
    }

    return refs;
}

std::vector<uint64_t> InstructionWrapper::GetDataReferences() const {
    if (!view) {
        return {};
    }
    return view->GetDataReferences(address);
}

std::string InstructionWrapper::GetText() const {
    std::string text;
    for (const auto& token : tokens) {
        text += token.text;
    }
    return text;
}

// Sol2 Binding Registration

void RegisterInstructionBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering Instruction bindings");

    lua.new_usertype<InstructionWrapper>(INSTRUCTION_METATABLE,
        // No public constructor - created from BasicBlock:disassembly() etc.
        sol::no_constructor,

        // Properties - simple getters as properties for clean API
        // CONSISTENCY: address returns HexAddress for smartlink support
        "address", sol::property([](const InstructionWrapper& i) -> HexAddress {
            return HexAddress(i.address);
        }),

        "mnemonic", sol::property([](const InstructionWrapper& i) -> std::string {
            return i.mnemonic;
        }),

        "length", sol::property([](const InstructionWrapper& i) -> size_t {
            return i.GetLength();
        }),

        "text", sol::property([](const InstructionWrapper& i) -> std::string {
            return i.GetText();
        }),

        "arch", sol::property([](const InstructionWrapper& i) -> std::string {
            return i.arch ? i.arch->GetName() : "<unknown>";
        }),

        // Methods - computed values that may do work
        "operands", [](const InstructionWrapper& i) -> std::vector<std::string> {
            return i.GetOperands();
        },

        "bytes", [](const InstructionWrapper& i) -> std::string {
            std::vector<uint8_t> bytes = i.GetBytes();
            return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        },

        // CONSISTENCY: references returns table with HexAddress arrays
        "references", [](sol::this_state ts, const InstructionWrapper& i) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();

            std::vector<uint64_t> codeRefs = i.GetCodeReferences();
            std::vector<uint64_t> dataRefs = i.GetDataReferences();

            sol::table codeTable = lua.create_table();
            for (size_t idx = 0; idx < codeRefs.size(); idx++) {
                codeTable[idx + 1] = HexAddress(codeRefs[idx]);
            }
            result["code"] = codeTable;

            sol::table dataTable = lua.create_table();
            for (size_t idx = 0; idx < dataRefs.size(); idx++) {
                dataTable[idx + 1] = HexAddress(dataRefs[idx]);
            }
            result["data"] = dataTable;

            return result;
        },

        // Equality based on address
        sol::meta_function::equal_to, [](const InstructionWrapper& a,
                                         const InstructionWrapper& b) -> bool {
            return a.address == b.address;
        },

        // String representation - returns the disassembled text
        sol::meta_function::to_string, [](const InstructionWrapper& i) -> std::string {
            return i.GetText();
        }
    );

    if (logger) logger->LogDebug("Instruction bindings registered");
}

} // namespace BinjaLua
