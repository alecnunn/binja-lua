# binja-lua

A _(highly experimental)_ Lua scripting provider for Binary Ninja.

This plugin adds Lua 5.4 as a scripting language in Binary Ninja, sitting alongside Python in the scripting console. The bindings aim to feel natural to Lua users while providing access to Binary Ninja's analysis capabilities.

The plugin version is available at runtime via the `binjalua` global table (`binjalua.version`, `binjalua.version_major`, etc.). See [docs/versioning.md](docs/versioning.md) for the semver policy.

## Quick Example

```lua
-- Get info about the current binary
print("Analyzing: " .. bv.filename)
print("Architecture: " .. bv.arch.name)

-- Iterate over functions
for _, func in ipairs(bv:functions()) do
    print(string.format("%s @ 0x%x", func.name, func.start_addr.value))

    -- Look at basic blocks
    for _, block in ipairs(func:basic_blocks()) do
        print(string.format("  block: 0x%x - 0x%x", block.start_addr.value, block.end_addr.value))
    end
end
```

## Installation

Download the latest release and copy the plugin files to your Binary Ninja plugins folder:
- Windows: `%APPDATA%\Binary Ninja\plugins\`
- macOS: `~/Library/Application Support/Binary Ninja/plugins/`
- Linux: `~/.binaryninja/plugins/`

_Optionally_ copy the `lua-api` directory into your plugins directory in a folder named `binja-lua` (e.g. `~/.binaryninja/plugins/binja-lua/lua-api`

To build from source, see [docs/building.md](docs/building.md).

## Usage

Once installed, you'll find Lua available in:
- **Scripting Console**: Select "Lua" from the dropdown in the console pane
- **Run Script**: Use `File > Run Lua Script...` to execute `.lua` files

### Magic Variables

When running in the console, these variables are automatically available based on your current view:

| Variable | Description |
|----------|-------------|
| `bv` | The current BinaryView |
| `here` | Current cursor address |
| `current_function` | Function at cursor (or nil) |
| `current_selection` | Selected address range |

When running scripts from file, `bv` is set to the active binary but cursor-based variables like `here` won't be available since there's no UI context.

### API Style

Properties are accessed directly, methods use the colon syntax:

```lua
-- Properties (no parentheses)
local name = func.name
local addr = func.start_addr

-- Methods (colon and parentheses)
func:set_name("my_function")
local blocks = func:basic_blocks()
local data = bv:read(addr, 16)
```

## What's Included

The bindings cover a substantial subset of the Binary Ninja API:

- **BinaryView**: File access, memory read/write, functions, symbols, sections, strings, metadata round-trip
- **Function**: Properties, basic blocks, IL access, parameters, variables, calling convention, platform, call graph
- **BasicBlock**: Dominators, control flow edges, instructions
- **Architecture / CallingConvention / Platform**: Registers, flags, calling conventions, platform-specific types
- **Types**: Structures, enums, function types
- **IL**: Per-instruction operand walking for `LowLevelILFunction` / `MediumLevelILFunction` / `HighLevelILFunction` with `LLILInstruction` / `MLILInstruction` / `HLILInstruction` value-usertypes (operands, detailed_operands, prefix_operands, traverse, SSA navigation, HLIL tree navigation)
- **Symbols & Tags**: Symbol CRUD with `Symbol.new` factory, TagType CRUD, user and auto tags
- **Settings**: Schema-aware settings with scope-aware typed get/set, serialization
- **FlowGraph**: Custom graph visualization with styled edges
- **Metadata**: Full round-trip including embedded-null strings and int64 precision

See [docs/api-reference.md](docs/api-reference.md) for the full API surface.

## License

BSD 3-Clause License. See [LICENSE.txt](LICENSE.txt) for details.
