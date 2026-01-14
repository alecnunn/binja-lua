# binja-lua

A _(hightly experimental)_ Lua scripting scripting for Binary Ninja.

This plugin adds Lua 5.4 as a scripting language in Binary Ninja, sitting alongside Python in the scripting console. The bindings aim to feel natural to Lua users while providing access to Binary Ninja's analysis capabilities.

## Quick Example

```lua
-- Get info about the current binary
print("Analyzing: " .. bv.filename)
print("Architecture: " .. bv.arch)

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

The bindings cover the core Binary Ninja API:

- **BinaryView**: File access, memory read/write, functions, symbols, sections, strings
- **Function**: Properties, basic blocks, IL access, parameters, variables, call graph
- **BasicBlock**: Dominators, control flow edges, instructions
- **Types**: Structures, enums, function types, type libraries
- **IL**: Low-level, medium-level, and high-level IL
- **Tags & Metadata**: Bookmarks, annotations, custom metadata storage
- **FlowGraph**: Custom graph visualization

See the `examples/` directory for practical usage patterns, or check [docs/api-reference.md](docs/api-reference.md) for the full API.

## License

BSD 3-Clause License. See [LICENSE.txt](LICENSE.txt) for details.
