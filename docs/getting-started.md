# Getting Started with Binary Ninja Lua Scripting

The Binja-Lua scripting provider enables powerful binary analysis through an intuitive Lua API.

## Installation

1. Build the plugin following the instructions in the README
2. Copy the built plugin to your Binary Ninja plugins directory
3. Restart Binary Ninja

## Quick Start

1. **Open Binary Ninja** with a binary file loaded
2. **Open the scripting console** (Tools -> Scripting Console)
3. **Select "Lua"** as the scripting language
4. **Start scripting**

## Your First Script

```lua
-- Get basic information about the loaded binary
print("File:", bv.file)
print("Architecture:", bv.arch)
print("Size:", bv.length, "bytes")
print("Functions:", #bv:functions())

-- Use the built-in table inspector for complex data
local info = {
    file = bv.file,
    arch = bv.arch,
    functions = #bv:functions()
}
dump(info)  -- Pretty prints the table
```

## Common Tasks

### List All Functions
```lua
local functions = bv:functions()
print("Found", #functions, "functions:")

for i = 1, math.min(10, #functions) do
    local func = functions[i]
    print(string.format("%2d. %-20s @ %s (%d bytes)",
          i, func.name, func.start_addr, func.size))
end
```

### Analyze Current Function
```lua
if current_function then
    local func = current_function
    print("Analyzing:", func.name)
    print("Address range:", func.start_addr, "-", func.end_addr)
    print("Size:", func.size, "bytes")

    local blocks = func:basic_blocks()
    print("Basic blocks:", #blocks)

    for i, block in ipairs(blocks) do
        print(string.format("  Block %d: %s (%d instructions)",
              i, block.start_addr, block:instruction_count()))
    end
end
```

### Examine Disassembly
```lua
if current_function then
    local blocks = current_function:basic_blocks()
    if #blocks > 0 then
        local first_block = blocks[1]
        local disasm = first_block:disassembly()

        print("Disassembly of first basic block:")
        for i, line in ipairs(disasm) do
            print(string.format("  %s: %s", line.addr, line.text))
            if i >= 5 then -- Show first 5 instructions
                print("  ... (" .. (#disasm - 5) .. " more)")
                break
            end
        end
    end
end
```

### Cross-Reference Analysis
```lua
-- Find what calls the current function
if current_function then
    local func_addr = current_function.start_addr.value
    local callers = bv:get_code_refs(func_addr)

    if #callers > 0 then
        print("Function is called by:")
        for _, caller in ipairs(callers) do
            print("  ", caller.addr)
        end
    else
        print("No callers found")
    end
end
```

### Memory Analysis
```lua
-- Read and display memory
local start = bv.start_addr.value
local data = bv:read(start, 32)

print("First 32 bytes:")
local hex = ""
for i = 1, #data do
    hex = hex .. string.format("%02x ", data:byte(i))
    if i % 16 == 0 then
        print(string.format("0x%08x: %s", start + i - 16, hex))
        hex = ""
    end
end
if #hex > 0 then
    print(string.format("0x%08x: %s", start + #data - (#data % 16), hex))
end
```

## Tips and Best Practices

### Error Handling
```lua
-- Always use pcall for potentially failing operations
local success, result = pcall(function()
    return bv:get_function_at(some_address)
end)

if success and result then
    print("Found function:", result.name)
else
    print("No function at address or error occurred")
end
```

### Address Formatting
```lua
-- HexAddress objects print in hex automatically
print("Current address:", here)  -- Prints as 0x...

-- For arithmetic, use .value to get the numeric value
local next_addr = here.value + 0x10
print(string.format("Next address: 0x%x", next_addr))
```

### Iterating Safely
```lua
-- When iterating over collections, check for nil/empty
local functions = bv:functions()
if functions and #functions > 0 then
    for i, func in ipairs(functions) do
        -- Process function
        print(i, func.name)
    end
else
    print("No functions found")
end
```

### Performance Considerations
```lua
-- Cache frequently accessed values
local funcs = bv:functions()
local func_count = #funcs

print("Analyzing", func_count, "functions...")
for i = 1, func_count do
    local func = funcs[i]
    -- Process func using properties: func.name, func.size, etc.
end
```

## API Conventions

Understanding the difference between properties and methods:

### Properties (use dot notation)
```lua
-- Properties are accessed with dot notation
print(bv.file)           -- File path
print(bv.arch)           -- Architecture name
print(bv.length)         -- Binary size
print(func.name)         -- Function name
print(func.size)         -- Function size
print(func.start_addr)   -- Function start (HexAddress)
print(block.start_addr)  -- Block start (HexAddress)
```

### Methods (use colon notation)
```lua
-- Methods are called with colon notation
local funcs = bv:functions()           -- Get all functions
local func = bv:get_function_at(addr)  -- Get function at address
local blocks = func:basic_blocks()     -- Get basic blocks
local count = block:instruction_count() -- Get instruction count
local disasm = block:disassembly()     -- Get disassembly
```

### HexAddress Values
```lua
-- HexAddress objects display in hex but need .value for arithmetic
local addr = func.start_addr
print(addr)              -- Prints "0x401000"
print(addr.value)        -- Prints 4198400 (decimal)
print(addr.value + 0x10) -- Arithmetic works with .value
```

## Magic Variables

The following variables are automatically available and updated based on Binary Ninja's UI context:

| Variable | Type | Description |
|----------|------|-------------|
| `bv` | BinaryView | Current binary view instance |
| `current_function` | Function | Currently selected function (may be nil) |
| `current_address` | HexAddress | Current address in the UI |
| `here` | HexAddress | Alias for current_address |
| `current_selection` | Selection | Currently selected range (may be nil) |

## Next Steps

- Explore the [API Reference](api-reference.md) for complete method documentation
- Check out the [Lua Extensions](lua-extensions.md) for fluent collection APIs
- See the `examples/` directory for more advanced usage

Happy analyzing!
