# Binary Ninja Lua API Reference

*Generated from API definitions*

## Table of Contents

- [HexAddress](#hexaddress)
- [Selection](#selection)
- [Section](#section)
- [Symbol](#symbol)
- [BinaryView](#binaryview)
- [Function](#function)
- [BasicBlock](#basicblock)
- [Instruction](#instruction)
- [Variable](#variable)
- [Llil](#llil)
- [Mlil](#mlil)
- [Hlil](#hlil)
- [FlowGraph](#flowgraph)
- [FlowGraphNode](#flowgraphnode)
- [DataVariable](#datavariable)
- [TagType](#tagtype)
- [Tag](#tag)
- [Type](#type)
- [GlobalFunctions](#globalfunctions)
- [Magic Variables](#magic-variables)

---

## HexAddress

*Wrapper type for addresses that displays in hexadecimal format. Supports arithmetic operations and comparisons. Use .value to get the raw numeric value for calculations.
*

### Properties

#### `HexAddress.value` -> `integer`

Raw numeric value of the address for arithmetic operations

**Example:**
```lua
local addr = bv.start_addr
local numeric = addr.value  -- Get as number
local next_page = addr.value + 0x1000
```

---

## Selection

*Represents a selected address range in the Binary Ninja UI. Available as current_selection magic variable when the user has selected a range.
*

### Properties

#### `Selection.start_addr` -> `HexAddress`

Start address of the selected range

**Example:**
```lua
if current_selection then
    print("Selection starts at:", current_selection.start_addr)
end
```

#### `Selection.end_addr` -> `HexAddress`

End address of the selected range (exclusive)

**Example:**
```lua
local sel = current_selection
print("Selected range:", sel.start_addr, "to", sel.end_addr)
```

#### `Selection.length` -> `integer`

Length of the selection in bytes

**Example:**
```lua
print("Selected", current_selection.length, "bytes")
```

---

## Section

*Represents a section in the binary (e.g., .text, .data, .rodata). Sections define regions of the binary with specific semantics and permissions.
*

### Properties

#### `Section.name` -> `string`

Name of the section (e.g., ".text", ".data", ".rodata")

**Example:**
```lua
for _, s in ipairs(bv:sections()) do
    print(s.name)
end
```

#### `Section.start_addr` -> `HexAddress`

Starting address of the section

**Example:**
```lua
local text = bv:sections()[1]
print("Section starts at:", text.start_addr)
```

#### `Section.length` -> `integer`

Size of the section in bytes

**Example:**
```lua
for _, s in ipairs(bv:sections()) do
    print(s.name, s.length, "bytes")
end
```

#### `Section.type` -> `string`

Section type - "code", "data", "external", or "default"

**Example:**
```lua
for _, s in ipairs(bv:sections()) do
    if s.type == "code" then
        print("Code section:", s.name)
    end
end
```

### Methods

#### `Section:permissions(...)` -> `table<{read: boolean, write: boolean, execute: boolean}>`

Get the section's memory permissions

**Example:**
```lua
local perms = section:permissions()
if perms.execute then
    print("Executable section")
end
```

---

## Symbol

*Represents a symbol (function name, variable name, import, export, etc.) in the binary. Symbols provide human-readable names for addresses.
*

### Properties

#### `Symbol.name` -> `string`

Full name of the symbol (may include namespace/mangling)

**Example:**
```lua
local sym = func.symbol
if sym then print("Full name:", sym.name) end
```

#### `Symbol.short_name` -> `string`

Demangled/shortened name suitable for display

**Example:**
```lua
for _, imp in ipairs(bv:imports()) do
    print(imp.short_name)
end
```

#### `Symbol.address` -> `HexAddress`

Address where the symbol is located

**Example:**
```lua
for _, exp in ipairs(bv:exports()) do
    print(exp.short_name, "@", exp.address)
end
```

#### `Symbol.type` -> `string`

Symbol type string - "Function", "ImportAddress", "ImportedFunction", "Data", "ImportedData", "External", "LibraryFunction", "LocalLabel", etc.


**Example:**
```lua
for _, sym in ipairs(bv:exports()) do
    print(sym.short_name, "type:", sym.type)
end
```

#### `Symbol.type_value` -> `integer`

Numeric symbol type value for programmatic comparisons

**Example:**
```lua
-- Use type string instead for readability
if sym.type == "Function" then ... end
```

---

## BinaryView

*Main interface for analyzing loaded binary files. Provides access to functions, memory, sections, symbols, and metadata. Available as the 'bv' magic variable.
*

### Properties

#### `BinaryView.start_addr` -> `HexAddress`

Starting address of the binary view's address space

**Aliases:** `start`

**Example:**
```lua
print(bv.start_addr)  -- e.g., 0x140000000
```

#### `BinaryView.start` -> `HexAddress`

Alias for start_addr

#### `BinaryView.end_addr` -> `HexAddress`

End address of the binary view (one past the last valid address)

**Aliases:** `end`

**Example:**
```lua
print(bv.end_addr)
```

#### `BinaryView.end` -> `HexAddress`

Alias for end_addr (note: use bv["end"] since 'end' is a Lua keyword)

#### `BinaryView.length` -> `integer`

Total size of the binary in bytes

**Example:**
```lua
print('Size:', bv.length, 'bytes')
```

#### `BinaryView.file` -> `string`

Full path to the loaded binary file

**Aliases:** `filename`

**Example:**
```lua
print('File:', bv.file)
```

#### `BinaryView.filename` -> `string`

Alias for file property

#### `BinaryView.arch` -> `string`

Architecture name (e.g., 'x86_64', 'x86', 'armv7', 'aarch64', 'thumb2')

**Example:**
```lua
print('Architecture:', bv.arch)
```

#### `BinaryView.entry_point` -> `HexAddress`

Entry point address of the binary (program start)

**Example:**
```lua
local entry_func = bv:get_function_at(bv.entry_point)
if entry_func then print("Entry:", entry_func.name) end
```

#### `BinaryView.has_data_vars` -> `boolean`

Whether the binary has any defined data variables

**Example:**
```lua
if bv.has_data_vars then
    print("Data variables:", #bv:data_vars())
end
```

### Methods

#### `BinaryView:functions(...)` -> `table<Function>`

Get all analyzed functions in the binary

**Example:**
```lua
local funcs = bv:functions()
print('Found', #funcs, 'functions')
for i = 1, math.min(5, #funcs) do
    print(funcs[i].name)
end
```

#### `BinaryView:sections(...)` -> `table<Section>`

Get all sections in the binary

**Example:**
```lua
for _, section in ipairs(bv:sections()) do
    print(section.name, section.start_addr, section.length)
end
```

#### `BinaryView:strings(...)` -> `table<{addr: HexAddress, length: integer, type: integer, value: string}>`

Get all strings found in the binary

**Example:**
```lua
for _, s in ipairs(bv:strings()) do
    if #s.value > 5 then  -- Skip short strings
        print(s.addr, s.value)
    end
end
```

#### `BinaryView:imports(...)` -> `table<Symbol>`

Get all imported symbols (functions and data from external libraries)

**Example:**
```lua
print("Imports:")
for _, imp in ipairs(bv:imports()) do
    print(" ", imp.short_name, "@", imp.address)
end
```

#### `BinaryView:exports(...)` -> `table<Symbol>`

Get all exported symbols (functions and data exposed by this binary)

**Example:**
```lua
print("Exports:")
for _, exp in ipairs(bv:exports()) do
    print(" ", exp.short_name, "@", exp.address)
end
```

#### `BinaryView:data_vars(...)` -> `table<DataVariable>`

Get all defined data variables (global variables, constants)

**Example:**
```lua
for _, var in ipairs(bv:data_vars()) do
    print(var.address, var.name or "<unnamed>", var.type)
end
```

#### `BinaryView:get_function_at(...)` -> `Function|nil`

Get the function that starts at the exact given address

**Parameters:**
- `addr` (HexAddress|integer) - Exact start address of the function

**Example:**
```lua
local func = bv:get_function_at(0x401000)
if func then
    print("Found:", func.name)
else
    print("No function starts at this address")
end
```

#### `BinaryView:get_data_var_at(...)` -> `DataVariable|nil`

Get the data variable defined at the given address

**Parameters:**
- `addr` (integer) - Address to look up

**Example:**
```lua
local var = bv:get_data_var_at(0x404000)
if var then print(var.type) end
```

#### `BinaryView:define_data_var(...)` -> `boolean`

Define a data variable at the given address (auto-discovered)

**Parameters:**
- `addr` (integer) - Address to define the variable at
- `type_str` (string) - Type string (e.g., 'int32_t', 'char*', 'void*')
- `name` (string) - (optional) Symbol name for the variable

**Example:**
```lua
bv:define_data_var(0x404000, "int32_t", "g_counter")
```

#### `BinaryView:define_user_data_var(...)` -> `DataVariable|nil`

Define a user data variable and return it (persists in analysis database)

**Parameters:**
- `addr` (integer) - Address to define the variable at
- `type_str` (string) - Type string
- `name` (string) - (optional) Symbol name for the variable

**Example:**
```lua
local var = bv:define_user_data_var(0x404000, "uint64_t", "counter")
if var then print("Created:", var.name) end
```

#### `BinaryView:undefine_data_var(...)`

Remove an auto-discovered data variable definition

**Parameters:**
- `addr` (integer) - Address of the variable to undefine

#### `BinaryView:undefine_user_data_var(...)`

Remove a user-defined data variable definition

**Parameters:**
- `addr` (integer) - Address of the variable to undefine

#### `BinaryView:get_next_data_var_after(...)` -> `DataVariable|nil`

Get the next data variable after the given address

**Parameters:**
- `addr` (integer) - Starting address to search from

**Example:**
```lua
local var = bv:get_next_data_var_after(here.value)
if var then print("Next var at:", var.address) end
```

#### `BinaryView:get_previous_data_var_before(...)` -> `DataVariable|nil`

Get the previous data variable before the given address

**Parameters:**
- `addr` (integer) - Starting address to search from

#### `BinaryView:get_next_function_start_after(...)` -> `HexAddress|nil`

Navigate to the next function after the given address

**Parameters:**
- `addr` (HexAddress|integer) - Starting address

**Example:**
```lua
local next_func = bv:get_next_function_start_after(here)
if next_func then print("Next function at:", next_func) end
```

#### `BinaryView:get_previous_function_start_before(...)` -> `HexAddress|nil`

Navigate to the previous function before the given address

**Parameters:**
- `addr` (HexAddress|integer) - Starting address

#### `BinaryView:get_next_basic_block_start_after(...)` -> `HexAddress|nil`

Navigate to the next basic block after the given address

**Parameters:**
- `addr` (HexAddress|integer) - Starting address

#### `BinaryView:get_previous_basic_block_start_before(...)` -> `HexAddress|nil`

Navigate to the previous basic block before the given address

**Parameters:**
- `addr` (HexAddress|integer) - Starting address

#### `BinaryView:get_next_data_after(...)` -> `HexAddress|nil`

Navigate to the next defined data item after the given address

**Parameters:**
- `addr` (HexAddress|integer) - Starting address

#### `BinaryView:get_previous_data_before(...)` -> `HexAddress|nil`

Navigate to the previous defined data item before the given address

**Parameters:**
- `addr` (HexAddress|integer) - Starting address

#### `BinaryView:get_functions_at(...)` -> `table<Function>`

Get all functions starting at the given address (handles overlapping functions)

**Parameters:**
- `addr` (HexAddress|integer) - Address to look up

**Example:**
```lua
-- Some addresses may have multiple functions (e.g., different architectures)
local funcs = bv:get_functions_at(here)
for _, f in ipairs(funcs) do print(f.name, f.arch) end
```

#### `BinaryView:get_functions_containing(...)` -> `table<Function>`

Get all functions whose address range contains the given address

**Parameters:**
- `addr` (HexAddress|integer) - Address that must be within the function

**Example:**
```lua
local funcs = bv:get_functions_containing(here)
for _, f in ipairs(funcs) do
    print("Address is in:", f.name)
end
```

#### `BinaryView:get_basic_blocks_starting_at(...)` -> `table<BasicBlock>`

Get all basic blocks that start at the given address

**Parameters:**
- `addr` (HexAddress|integer) - Address to look up

#### `BinaryView:get_functions_by_name(...)` -> `table<Function>`

Search for functions by name (exact or substring match)

**Parameters:**
- `name` (string) - Function name or substring to search for
- `exact` (boolean) - (optional, default true) If true, match exact name; if false, substring match

**Example:**
```lua
-- Find main function
local mains = bv:get_functions_by_name("main", true)

-- Find all sub_ functions
local subs = bv:get_functions_by_name("sub_", false)
print("Found", #subs, "unnamed functions")
```

#### `BinaryView:find_next_data(...)` -> `HexAddress|nil`

Search for a byte pattern starting from the given address

**Parameters:**
- `start` (HexAddress|integer) - Address to start searching from
- `data` (string) - Byte pattern to search for (Lua string with raw bytes)

**Example:**
```lua
-- Search for NOP sled
local found = bv:find_next_data(bv.start_addr, "\x90\x90\x90\x90")
if found then print("NOP sled at:", found) end

-- Search for magic bytes
local mz = bv:find_next_data(bv.start_addr, "MZ")
```

#### `BinaryView:find_all_data(...)` -> `table<HexAddress>`

Find all occurrences of a byte pattern in the given range

**Parameters:**
- `start` (HexAddress|integer) - Start address of search range
- `end_addr` (HexAddress|integer) - End address of search range
- `data` (string) - Byte pattern to search for

**Example:**
```lua
-- Find all null dwords in first 0x10000 bytes
local nulls = bv:find_all_data(bv.start_addr, bv.start_addr.value + 0x10000, "\x00\x00\x00\x00")
print("Found", #nulls, "null dwords")
```

#### `BinaryView:find_next_text(...)` -> `HexAddress|nil`

Search for a text string in the binary data

**Parameters:**
- `start` (HexAddress|integer) - Address to start searching from
- `pattern` (string) - Text string to search for

**Example:**
```lua
local pwd = bv:find_next_text(bv.start_addr, "password")
if pwd then print("Found 'password' at:", pwd) end
```

#### `BinaryView:find_all_text(...)` -> `table<{addr: HexAddress, match: string}>`

Find all occurrences of a text string in the given range

**Parameters:**
- `start` (HexAddress|integer) - Start address of search range
- `end_addr` (HexAddress|integer) - End address of search range
- `pattern` (string) - Text string to search for

**Example:**
```lua
local results = bv:find_all_text(bv.start_addr, bv.end_addr, "error")
for _, r in ipairs(results) do
    print(r.addr, r.match)
end
```

#### `BinaryView:find_next_constant(...)` -> `HexAddress|nil`

Search for a constant value used in disassembly instructions

**Parameters:**
- `start` (HexAddress|integer) - Address to start searching from
- `constant` (integer) - Constant value to search for (e.g., immediate operand)

**Example:**
```lua
-- Find instructions using magic constant
local found = bv:find_next_constant(bv.start_addr, 0xDEADBEEF)
if found then print("Magic constant used at:", found) end
```

#### `BinaryView:read(...)` -> `string`

Read raw bytes from the binary at the given address

**Parameters:**
- `addr` (integer) - Address to read from
- `len` (integer) - Number of bytes to read

**Example:**
```lua
-- Read and hexdump 16 bytes
local data = bv:read(bv.entry_point.value, 16)
local hex = ""
for i = 1, #data do
    hex = hex .. string.format("%02x ", data:byte(i))
end
print(hex)
```

#### `BinaryView:get_code_refs(...)` -> `table<{addr: HexAddress, func: Function|nil}>`

Get all code references TO the given address (who calls/jumps here?)

**Parameters:**
- `addr` (integer) - Target address to find references to

**Example:**
```lua
local refs = bv:get_code_refs(func.start.value)
print("Function is called from", #refs, "locations:")
for _, ref in ipairs(refs) do
    print("  ", ref.addr, ref.func and ref.func.name or "")
end
```

#### `BinaryView:get_data_refs(...)` -> `table<HexAddress>`

Get all data references TO the given address (who reads/writes here?)

**Parameters:**
- `addr` (integer) - Target address to find references to

**Example:**
```lua
local refs = bv:get_data_refs(0x404000)
print("Data is accessed from:")
for _, addr in ipairs(refs) do print("  ", addr) end
```

#### `BinaryView:get_code_refs_from(...)` -> `table<HexAddress>`

Get all code references FROM the given address (what does this call/jump to?)

**Parameters:**
- `addr` (integer) - Source address to find outgoing references from

**Example:**
```lua
local targets = bv:get_code_refs_from(here.value)
for _, target in ipairs(targets) do
    print("References:", target)
end
```

#### `BinaryView:get_data_refs_from(...)` -> `table<HexAddress>`

Get all data references FROM the given address (what data does this access?)

**Parameters:**
- `addr` (integer) - Source address to find outgoing references from

#### `BinaryView:get_callers(...)` -> `table<{address: HexAddress, func: Function|nil, arch: string|nil}>`

Get all call sites that call the function at the given address

**Parameters:**
- `addr` (integer) - Function address to find callers of

**Example:**
```lua
local callers = bv:get_callers(func.start.value)
print("Called from", #callers, "locations:")
for _, caller in ipairs(callers) do
    print("  ", caller.address, caller.func and caller.func.name or "")
end
```

#### `BinaryView:get_callees(...)` -> `table<HexAddress>`

Get all addresses that are called from the given address

**Parameters:**
- `addr` (integer) - Address (within a function) to find outgoing calls from

#### `BinaryView:comment_at_address(...)` -> `string`

Get the comment at the given address

**Parameters:**
- `addr` (integer) - Address to get comment for

**Example:**
```lua
local comment = bv:comment_at_address(here.value)
if comment ~= "" then print("Comment:", comment) end
```

#### `BinaryView:set_comment_at_address(...)` -> `boolean`

Set or update a comment at the given address

**Parameters:**
- `addr` (integer) - Address to set comment at
- `comment` (string) - Comment text (empty string removes the comment)

**Example:**
```lua
bv:set_comment_at_address(here.value, "Interesting code here")
```

#### `BinaryView:tag_types(...)` -> `table<TagType>`

Get all tag types defined in this binary view

**Example:**
```lua
for _, tt in ipairs(bv:tag_types()) do
    print(tt.name, tt.icon)
end
```

#### `BinaryView:get_tag_type(...)` -> `TagType|nil`

Get a tag type by its name

**Parameters:**
- `name` (string) - Name of the tag type to look up

#### `BinaryView:create_tag_type(...)` -> `TagType`

Create a new tag type for annotating the binary

**Parameters:**
- `name` (string) - Name for the tag type
- `icon` (string) - Icon/emoji for the tag type (single character)

**Example:**
```lua
local vuln = bv:create_tag_type("Vulnerability", "!")
local note = bv:create_tag_type("Note", "*")
```

#### `BinaryView:remove_tag_type(...)`

Remove a tag type (and all tags of that type)

**Parameters:**
- `tagType` (TagType) - Tag type to remove

#### `BinaryView:get_tags_at(...)` -> `table<Tag>`

Get all tags at the given address

**Parameters:**
- `addr` (HexAddress|integer) - Address to get tags at

**Example:**
```lua
local tags = bv:get_tags_at(here)
for _, tag in ipairs(tags) do
    print(tag.type.name, tag.data)
end
```

#### `BinaryView:add_tag(...)`

Add an existing tag to an address

**Parameters:**
- `addr` (HexAddress|integer) - Address to add tag at
- `tag` (Tag) - Tag object to add
- `user` (boolean) - (optional, default true) Whether this is a user-defined tag

#### `BinaryView:remove_tag(...)`

Remove a tag from an address

**Parameters:**
- `addr` (HexAddress|integer) - Address to remove tag from
- `tag` (Tag) - Tag object to remove
- `user` (boolean) - (optional, default true) Whether this is a user-defined tag

#### `BinaryView:create_user_tag(...)` -> `Tag|nil`

Create and add a new tag at an address in one step

**Parameters:**
- `addr` (HexAddress|integer) - Address to create tag at
- `tagTypeName` (string) - Name of the tag type (must already exist)
- `data` (string) - Tag data/description

**Example:**
```lua
bv:create_user_tag(here, "Vulnerability", "Buffer overflow in memcpy")
```

#### `BinaryView:get_all_tags(...)` -> `table<{addr: HexAddress, tag: Tag, auto: boolean, func: Function|nil}>`

Get all tags in the entire binary view

**Example:**
```lua
local all_tags = bv:get_all_tags()
print("Total tags:", #all_tags)
```

#### `BinaryView:get_tags_of_type(...)` -> `table<{addr: HexAddress, tag: Tag, auto: boolean, func: Function|nil}>`

Get all tags of a specific type

**Parameters:**
- `tagType` (TagType) - Tag type to filter by

#### `BinaryView:get_tags_in_range(...)` -> `table<{addr: HexAddress, tag: Tag, auto: boolean}>`

Get all tags within a specified address range

**Parameters:**
- `start_addr` (HexAddress|integer) - Start address of the range to search
- `end_addr` (HexAddress|integer) - End address of the range (exclusive)
- `user_only` (boolean) - (optional, default false) If true, only return user-defined tags

**Example:**
```lua
local tags = bv:get_tags_in_range(func.start, func["end"])
for _, entry in ipairs(tags) do
    print(entry.addr, entry.tag.type.name, entry.tag.data)
end
```

#### `BinaryView:run_transaction(...)` -> `boolean`

Run multiple operations as a single undoable transaction

**Parameters:**
- `func` (function) - Function to run; should return true on success, false to abort

**Example:**
```lua
local success = bv:run_transaction(function()
    bv:set_comment_at_address(0x401000, "Modified")
    bv:set_comment_at_address(0x401010, "Also modified")
    return true  -- Commit the transaction
end)
```

#### `BinaryView:can_undo(...)` -> `boolean`

Check if undo is available

**Example:**
```lua
if bv:can_undo() then print("Can undo") end
```

#### `BinaryView:undo(...)` -> `boolean`

Undo the last action

#### `BinaryView:can_redo(...)` -> `boolean`

Check if redo is available

#### `BinaryView:redo(...)` -> `boolean`

Redo the last undone action

#### `BinaryView:get_type_by_name(...)` -> `Type|nil`

Look up a type by its name

**Parameters:**
- `name` (string) - Type name to look up (e.g., "HANDLE", "size_t")

**Example:**
```lua
local t = bv:get_type_by_name("HANDLE")
if t then print("Found type:", t) end
```

#### `BinaryView:get_type_by_id(...)` -> `Type|nil`

Look up a type by its internal ID

**Parameters:**
- `id` (string) - Internal type ID

#### `BinaryView:get_type_id(...)` -> `string`

Get the internal ID for a type name

**Parameters:**
- `name` (string) - Type name

#### `BinaryView:types(...)` -> `table<{name: string, type: Type}>`

Get all types defined in the binary

**Example:**
```lua
for _, entry in ipairs(bv:types()) do
    print(entry.name)
end
```

#### `BinaryView:define_user_type(...)` -> `boolean`

Define a new user type from a C-style type string

**Parameters:**
- `name` (string) - Name for the new type
- `type_str` (string) - C-style type definition

**Example:**
```lua
bv:define_user_type("Point", "struct { int x; int y; }")
bv:define_user_type("Callback", "void (*)(int, void*)")
```

#### `BinaryView:undefine_user_type(...)`

Remove a user-defined type

**Parameters:**
- `name` (string) - Name of the type to remove

#### `BinaryView:parse_type_string(...)` -> `Type|nil, string`

Parse a C-style type string without defining it

**Parameters:**
- `type_str` (string) - C-style type string to parse

**Example:**
```lua
local t, err = bv:parse_type_string("int (*)(void*, size_t)")
if t then
    print("Parsed successfully")
else
    print("Error:", err)
end
```

#### `BinaryView:get_type_refs_for_type(...)` -> `table<{name: string, offset: integer, ref_type: string}>`

Find all references to a named type

**Parameters:**
- `typeName` (string) - Name of the type to find references for

#### `BinaryView:get_outgoing_type_refs(...)` -> `table<string>`

Get types directly referenced by the given type

**Parameters:**
- `typeName` (string) - Name of the type to analyze

#### `BinaryView:get_outgoing_recursive_type_refs(...)` -> `table<string>`

Get all types referenced by the given type (recursively follows nested types)

**Parameters:**
- `typeName` (string) - Name of the type to analyze

#### `BinaryView:get_analysis_progress(...)` -> `table<{state: string, count: integer, total: integer}>`

Get current analysis progress information including state, count, and total items

**Example:**
```lua
local progress = bv:get_analysis_progress()
print("State:", progress.state)  -- "idle", "initial", "hold", "discovery", etc.
if progress.state ~= "idle" then
    print(string.format("Progress: %d/%d", progress.count, progress.total))
end
```

#### `BinaryView:update_analysis(...)`

Trigger a background re-analysis of the binary. Returns immediately while analysis runs.

**Example:**
```lua
func:set_return_type("int")
bv:update_analysis()  -- Queue re-analysis
```

#### `BinaryView:update_analysis_and_wait(...)`

Trigger re-analysis and wait for it to complete before returning

**Example:**
```lua
bv:define_user_type("MyStruct", "struct { int x; int y; }")
bv:update_analysis_and_wait()
print("Analysis complete!")
```

#### `BinaryView:abort_analysis(...)`

Cancel any currently running analysis

**Example:**
```lua
if bv.analysis_progress.state == "running" then
    bv:abort_analysis()
end
```

#### `BinaryView:store_metadata(...)`

Store a value in the binary's metadata database. Supports booleans, strings, integers, doubles, and tables.

**Parameters:**
- `key` (string) - Unique key to store the value under
- `value` (any) - Value to store (boolean, string, integer, number, or table)
- `isAuto` (boolean) - (optional, default false) If true, mark as auto-generated (not user-defined)

**Example:**
```lua
bv:store_metadata("my_script.last_run", os.time())
bv:store_metadata("my_script.config", {enabled = true, threshold = 100})
```

#### `BinaryView:query_metadata(...)` -> `any|nil`

Retrieve a previously stored metadata value

**Parameters:**
- `key` (string) - Key to look up

**Example:**
```lua
local last_run = bv:query_metadata("my_script.last_run")
if last_run then print("Last run:", last_run) end
```

#### `BinaryView:remove_metadata(...)`

Remove a metadata key and its value from the database

**Parameters:**
- `key` (string) - Key to remove

**Example:**
```lua
bv:remove_metadata("my_script.temp_data")
```

#### `BinaryView:show_plain_text_report(...)`

Display a plain text report in Binary Ninja's report panel

**Parameters:**
- `title` (string) - Title for the report window
- `contents` (string) - Plain text content to display

**Example:**
```lua
local report = "Analysis Report\n===============\n"
report = report .. "Functions: " .. bv.function_count .. "\n"
bv:show_plain_text_report("My Analysis", report)
```

#### `BinaryView:show_markdown_report(...)`

Display a markdown-formatted report in Binary Ninja's report panel

**Parameters:**
- `title` (string) - Title for the report window
- `contents` (string) - Markdown content to display
- `plaintext` (string) - (optional) Plain text fallback for copying

**Example:**
```lua
local md = "# Analysis Report\n\n"
md = md .. "**Functions:** " .. bv.function_count .. "\n"
md = md .. "| Address | Name |\n|---------|------|\n"
bv:show_markdown_report("My Analysis", md)
```

#### `BinaryView:show_html_report(...)`

Display an HTML report in Binary Ninja's report panel

**Parameters:**
- `title` (string) - Title for the report window
- `contents` (string) - HTML content to display
- `plaintext` (string) - (optional) Plain text fallback for copying

**Example:**
```lua
local html = "<h1>Report</h1><p>Functions: " .. bv.function_count .. "</p>"
bv:show_html_report("My Analysis", html)
```

#### `BinaryView:show_graph_report(...)`

Display a FlowGraph in Binary Ninja's graph view

**Parameters:**
- `title` (string) - Title for the graph window
- `graph` (FlowGraph) - FlowGraph object to display

**Example:**
```lua
local func = bv:get_function_at(bv.entry_point)
if func then
    local graph = func:create_graph()
    bv:show_graph_report("Entry Point CFG", graph)
end
```

---

## Function

*Represents a single function in the binary. Provides access to basic blocks, disassembly, variables, IL representations, and analysis metadata.
*

### Properties

#### `Function.start_addr` -> `HexAddress`

Start address of the function

**Aliases:** `start`

#### `Function.start` -> `HexAddress`

Start address of the function (preferred name)

**Example:**
```lua
print("Function starts at:", func.start)
```

#### `Function.end_addr` -> `HexAddress`

End address of the function (highest address in function)

**Aliases:** `end, highest_addr`

#### `Function.end` -> `HexAddress`

End address of the function (use func["end"] since 'end' is a Lua keyword)

#### `Function.size` -> `integer`

Size of the function in bytes (end - start)

**Example:**
```lua
print("Function size:", func.size, "bytes")
```

#### `Function.name` -> `string`

Display name of the function

**Example:**
```lua
print("Analyzing:", func.name)
```

#### `Function.arch` -> `string`

Architecture name for this function (e.g., "x86_64", "thumb2")

**Example:**
```lua
print("Architecture:", func.arch)
```

#### `Function.comment` -> `string`

Function-level comment

**Example:**
```lua
if func.comment ~= "" then
    print("Comment:", func.comment)
end
```

#### `Function.symbol` -> `Symbol|nil`

Symbol associated with this function (may be nil for unnamed functions)

**Example:**
```lua
if func.symbol then
    print("Symbol:", func.symbol.name)
end
```

#### `Function.view` -> `BinaryView`

BinaryView containing this function

**Example:**
```lua
local bv = func.view
print("From file:", bv.file)
```

#### `Function.auto_discovered` -> `boolean`

Whether this function was auto-discovered by analysis (vs user-defined)

#### `Function.has_user_annotations` -> `boolean`

Whether the function has any user-added annotations

#### `Function.is_pure` -> `boolean`

Whether this function is marked as pure (no side effects, result depends only on inputs)

#### `Function.has_explicitly_defined_type` -> `boolean`

Whether the function signature was explicitly defined by user or type library

#### `Function.has_user_type` -> `boolean`

Whether the function has a user-defined type signature

#### `Function.has_unresolved_indirect_branches` -> `boolean`

Whether the function has indirect jumps/calls that couldn't be resolved

#### `Function.analysis_skipped` -> `boolean`

Whether analysis was skipped for this function

#### `Function.too_large` -> `boolean`

Whether the function was considered too large for full analysis

#### `Function.needs_update` -> `boolean`

Whether the function's analysis is out of date and needs re-analysis

#### `Function.analysis_skip_reason` -> `string`

Human-readable reason why analysis was skipped (if applicable)

#### `Function.can_return` -> `boolean`

Whether the function can return normally (false for noreturn functions)

**Example:**
```lua
if not func.can_return then
    print("This is a noreturn function")
end
```

#### `Function.auto` -> `boolean`

Whether this function was auto-discovered during analysis (same as auto_discovered)

**Example:**
```lua
if func.auto then print("Auto-discovered function") end
```

#### `Function.is_exported` -> `boolean`

Whether this function is exported (visible in the binary's export table)

**Example:**
```lua
for _, f in ipairs(bv:functions()) do
    if f.is_exported then print("Exported:", f.name) end
end
```

#### `Function.is_inlined_during_analysis` -> `boolean`

Whether this function is marked to be inlined during analysis

**Example:**
```lua
if func.is_inlined_during_analysis then
    print("Function will be inlined")
end
```

#### `Function.is_thunk` -> `boolean`

Whether this is a thunk function (simple jump to another function)

**Example:**
```lua
if func.is_thunk then
    print("This is a thunk/stub function")
end
```

#### `Function.stack_adjustment` -> `integer`

Stack adjustment made by this function in bytes

### Methods

#### `Function:basic_blocks(...)` -> `table<BasicBlock>`

Get all basic blocks in this function

**Example:**
```lua
local blocks = func:basic_blocks()
print("Function has", #blocks, "basic blocks")
for i, bb in ipairs(blocks) do
    print(string.format("  Block %d: %s - %s", i, bb.start_addr, bb.end_addr))
end
```

#### `Function:calls(...)` -> `table<{addr: HexAddress, target: HexAddress}>`

Get all call instructions in this function as address/target pairs

**Example:**
```lua
for _, call in ipairs(func:calls()) do
    print("Call at", call.addr, "to", call.target)
end
```

#### `Function:callers(...)` -> `table<Function>`

Get all functions that call this function

**Example:**
```lua
local callers = func:callers()
print("Called by", #callers, "functions:")
for _, f in ipairs(callers) do print("  ", f.name) end
```

#### `Function:call_sites(...)` -> `table<{address: HexAddress, func: Function|nil}>`

Get all locations within this function where calls are made

**Example:**
```lua
for _, site in ipairs(func:call_sites()) do
    print("Call at:", site.address)
end
```

#### `Function:callees(...)` -> `table<Function>`

Get all functions that this function calls

**Example:**
```lua
local callees = func:callees()
print("Calls", #callees, "functions:")
for _, f in ipairs(callees) do print("  ", f.name) end
```

#### `Function:callee_addresses(...)` -> `table<HexAddress>`

Get addresses of all functions called by this function

**Example:**
```lua
for _, addr in ipairs(func:callee_addresses()) do
    print("Calls:", addr)
end
```

#### `Function:caller_sites(...)` -> `table<{address: HexAddress, func: Function|nil, arch: string|nil}>`

Get all call sites that call this function

**Example:**
```lua
local sites = func:caller_sites()
print("Called from", #sites, "locations")
```

#### `Function:variables(...)` -> `table<Variable>`

Get all variables in the function (parameters and locals)

**Example:**
```lua
for _, var in ipairs(func:variables()) do
    print(var.name, var.type_name, var.source_type)
end
```

#### `Function:parameter_vars(...)` -> `table<Variable>`

Get only parameter variables

**Example:**
```lua
print("Parameters:")
for i, p in ipairs(func:parameter_vars()) do
    print(string.format("  %d. %s: %s", i, p.name, p.type_name))
end
```

#### `Function:clobbered_regs(...)` -> `table<string>`

Get registers that are clobbered (modified) by this function

**Example:**
```lua
print("Clobbered registers:", table.concat(func:clobbered_regs(), ", "))
```

#### `Function:get_variable_by_name(...)` -> `Variable|nil`

Look up a variable by name

**Parameters:**
- `name` (string) - Variable name to look up

**Example:**
```lua
local arg0 = func:get_variable_by_name("arg0")
if arg0 then print("Found:", arg0.type_name) end
```

#### `Function:create_user_var(...)` -> `boolean`

Create a new user-defined variable

**Parameters:**
- `storage` (integer) - Storage location (register number or stack offset)
- `typeStr` (string) - Type string for the variable
- `name` (string) - Name for the variable

#### `Function:delete_user_var(...)`

Delete a user-defined variable

**Parameters:**
- `var` (Variable) - Variable to delete

#### `Function:comment_at_address(...)` -> `string`

Get the comment at a specific address within this function

**Parameters:**
- `addr` (integer) - Address to get comment for

#### `Function:set_comment(...)` -> `boolean`

Set the function-level comment

**Parameters:**
- `comment` (string) - Comment text (empty to remove)

**Example:**
```lua
func:set_comment("Main entry point - initializes subsystems")
```

#### `Function:set_comment_at_address(...)` -> `boolean`

Set a comment at a specific address within this function

**Parameters:**
- `addr` (integer) - Address to set comment at
- `comment` (string) - Comment text (empty to remove)

#### `Function:set_name(...)` -> `boolean`

Rename the function

**Parameters:**
- `name` (string) - New name for the function

**Example:**
```lua
func:set_name("initialize_crypto")
```

#### `Function:set_return_type(...)` -> `boolean`

Set the function's return type

**Parameters:**
- `type_str` (string) - C-style return type (e.g., "int", "void*", "struct foo*")

**Example:**
```lua
func:set_return_type("int64_t")
```

#### `Function:set_parameter_type(...)` -> `boolean`

Change the type of a function parameter

**Parameters:**
- `index` (integer) - Parameter index (0-based)
- `type_str` (string) - New type string

**Example:**
```lua
func:set_parameter_type(0, "const char*")
```

#### `Function:set_parameter_name(...)` -> `boolean`

Rename a function parameter

**Parameters:**
- `index` (integer) - Parameter index (0-based)
- `name` (string) - New parameter name

**Example:**
```lua
func:set_parameter_name(0, "filename")
func:set_parameter_name(1, "mode")
```

#### `Function:add_parameter(...)` -> `boolean`

Add a new parameter to the function signature

**Parameters:**
- `type_str` (string) - Type string for the new parameter
- `name` (string) - (optional) Name for the new parameter

**Example:**
```lua
func:add_parameter("void*", "user_context")
```

#### `Function:remove_parameter(...)` -> `boolean`

Remove a parameter from the function signature

**Parameters:**
- `index` (integer) - Index of parameter to remove (0-based)

**Example:**
```lua
func:remove_parameter(2)  -- Remove third parameter
```

#### `Function:set_calling_convention(...)` -> `boolean`

Set the function's calling convention

**Parameters:**
- `ccName` (string) - Calling convention name (e.g., "cdecl", "stdcall", "fastcall")
- `reanalyze` (boolean) - (optional) Whether to trigger re-analysis after change

**Example:**
```lua
func:set_calling_convention("stdcall", true)
```

#### `Function:set_can_return(...)`

Mark whether the function can return

**Parameters:**
- `canReturn` (boolean) - True if function returns, false for noreturn
- `reanalyze` (boolean) - (optional) Whether to trigger re-analysis

**Example:**
```lua
func:set_can_return(false)  -- Mark as noreturn
```

#### `Function:set_has_variable_arguments(...)`

Mark whether the function accepts variable arguments (varargs)

**Parameters:**
- `hasVarArgs` (boolean) - True if function is variadic (like printf)
- `reanalyze` (boolean) - (optional) Whether to trigger re-analysis

#### `Function:set_is_pure(...)`

Mark whether the function is pure (no side effects)

**Parameters:**
- `isPure` (boolean) - True if function is pure
- `reanalyze` (boolean) - (optional) Whether to trigger re-analysis

#### `Function:set_analysis_skipped(...)`

Skip or unskip analysis for this function

**Parameters:**
- `skip` (boolean) - True to skip analysis, false to enable

#### `Function:set_user_inlined(...)`

Mark the function to be inlined during analysis at call sites

**Parameters:**
- `inlined` (boolean) - True to inline this function, false to disable inlining
- `reanalyze` (boolean) - (optional, default false) Whether to trigger re-analysis after change

**Example:**
```lua
-- Mark small utility function to be inlined
func:set_user_inlined(true, true)
```

#### `Function:reanalyze(...)`

Trigger re-analysis of this function

**Example:**
```lua
func:set_return_type("int")
func:reanalyze()
```

#### `Function:mark_updates_required(...)`

Mark that this function needs re-analysis on next analysis pass

#### `Function:get_llil(...)` -> `LowLevelILFunction|nil`

Get the Low Level IL representation of this function

**Example:**
```lua
local llil = func:get_llil()
if llil then
    print("LLIL instructions:", llil.instruction_count)
end
```

#### `Function:get_mlil(...)` -> `MediumLevelILFunction|nil`

Get the Medium Level IL representation of this function

**Example:**
```lua
local mlil = func:get_mlil()
if mlil then
    print("MLIL instructions:", mlil.instruction_count)
end
```

#### `Function:get_hlil(...)` -> `HighLevelILFunction|nil`

Get the High Level IL representation of this function

**Example:**
```lua
local hlil = func:get_hlil()
if hlil then
    print("HLIL instructions:", hlil.instruction_count)
end
```

#### `Function:type(...)` -> `table<{return_type: string, parameters: table, calling_convention: string|nil}>`

Get detailed function type information

**Example:**
```lua
local ft = func:type()
print("Returns:", ft.return_type)
for i, p in ipairs(ft.parameters) do
    print(string.format("  Param %d: %s %s", i, p.type, p.name or ""))
end
```

#### `Function:disassembly(...)` -> `table<{addr: HexAddress, text: string}>`

Get full disassembly of the function

**Example:**
```lua
for _, line in ipairs(func:disassembly()) do
    print(string.format("%s: %s", line.addr, line.text))
end
```

#### `Function:control_flow_graph(...)` -> `table<{blocks: table, edges: table}>`

Get control flow graph data for visualization

#### `Function:stack_layout(...)` -> `table<{offset: integer, name: string, type: string}>`

Get the stack frame layout

**Example:**
```lua
for _, var in ipairs(func:stack_layout()) do
    print(string.format("[%+d] %s: %s", var.offset, var.name, var.type))
end
```

#### `Function:merged_vars(...)` -> `table<{target: Variable, sources: table<Variable>}>`

Get merged variable groups from SSA analysis

**Example:**
```lua
for _, group in ipairs(func:merged_vars()) do
    print("Merged into:", group.target.name)
    for _, src in ipairs(group.sources) do
        print("  from:", src.name)
    end
end
```

#### `Function:split_vars(...)` -> `table<Variable>`

Get variables that have been split by SSA analysis

#### `Function:split_variable(...)`

Mark a variable to be split for better analysis

**Parameters:**
- `var` (Variable) - Variable to split

#### `Function:get_mlil_var_refs(...)` -> `table<{address: HexAddress, index: integer}>`

Get all references to a variable in the Medium Level IL

**Parameters:**
- `var` (Variable) - Variable to find references for

**Example:**
```lua
local var = func:variables()[1]
local refs = func:get_mlil_var_refs(var)
for _, ref in ipairs(refs) do
    print("MLIL ref at:", ref.address, "index:", ref.index)
end
```

#### `Function:get_hlil_var_refs(...)` -> `table<{address: HexAddress, index: integer}>`

Get all references to a variable in the High Level IL

**Parameters:**
- `var` (Variable) - Variable to find references for

**Example:**
```lua
local var = func:variables()[1]
local refs = func:get_hlil_var_refs(var)
for _, ref in ipairs(refs) do
    print("HLIL ref at:", ref.address, "index:", ref.index)
end
```

#### `Function:get_tags(...)` -> `table<Tag>`

Get all tags associated with this function

#### `Function:get_tags_at(...)` -> `table<Tag>`

Get tags at a specific address within this function

**Parameters:**
- `addr` (integer) - Address to get tags at

#### `Function:add_tag(...)`

Add a tag at an address within this function

**Parameters:**
- `addr` (integer) - Address to add tag at
- `tag` (Tag) - Tag to add
- `user` (boolean) - (optional) Whether this is a user tag

#### `Function:remove_tag(...)`

Remove a tag from an address within this function

**Parameters:**
- `addr` (integer) - Address to remove tag from
- `tag` (Tag) - Tag to remove
- `user` (boolean) - (optional) Whether this is a user tag

#### `Function:create_user_tag(...)` -> `Tag`

Create and add a tag at an address within this function

**Parameters:**
- `addr` (integer) - Address to create tag at
- `tagTypeName` (string) - Name of the tag type
- `data` (string) - Tag data/description

#### `Function:store_metadata(...)`

Store a value in the function's metadata database. Supports booleans, strings, integers, doubles, and tables.

**Parameters:**
- `key` (string) - Unique key to store the value under
- `value` (any) - Value to store (boolean, string, integer, number, or table)
- `isAuto` (boolean) - (optional, default false) If true, mark as auto-generated (not user-defined)

**Example:**
```lua
func:store_metadata("analyzed_by", "my_script")
func:store_metadata("complexity", {blocks = 10, edges = 15})
```

#### `Function:query_metadata(...)` -> `any|nil`

Retrieve a previously stored metadata value from this function

**Parameters:**
- `key` (string) - Key to look up

**Example:**
```lua
local analyzer = func:query_metadata("analyzed_by")
if analyzer then print("Analyzed by:", analyzer) end
```

#### `Function:remove_metadata(...)`

Remove a metadata key and its value from the function

**Parameters:**
- `key` (string) - Key to remove

**Example:**
```lua
func:remove_metadata("temp_analysis_data")
```

#### `Function:create_graph(...)` -> `FlowGraph`

Create a control flow graph for this function

**Parameters:**
- `type` (string) - (optional) Graph type - "normal", "llil", "mlil", or "hlil" (default "normal")

**Example:**
```lua
local graph = func:create_graph()
bv:show_graph_report("CFG", graph)

-- Create HLIL graph
local hlil_graph = func:create_graph("hlil")
bv:show_graph_report("HLIL CFG", hlil_graph)
```

#### `Function:create_graph_immediate(...)` -> `FlowGraph`

Create a control flow graph and wait for layout to complete before returning

**Parameters:**
- `type` (string) - (optional) Graph type - "normal", "llil", "mlil", or "hlil" (default "normal")

**Example:**
```lua
-- Get graph with layout already computed
local graph = func:create_graph_immediate()
print("Graph size:", graph.width, "x", graph.height)
```

---

## BasicBlock

*Represents a basic block within a function. A basic block is a sequence of instructions with a single entry point and single exit point - execution enters at the top and exits at the bottom, with no branches in between.
*

### Properties

#### `BasicBlock.start_addr` -> `HexAddress`

Start address of the basic block

**Example:**
```lua
print("Block starts at:", bb.start_addr)
```

#### `BasicBlock.end_addr` -> `HexAddress`

End address of the basic block (address after last instruction)

**Example:**
```lua
print("Block ends at:", bb.end_addr)
```

#### `BasicBlock.length` -> `integer`

Size of the basic block in bytes

**Example:**
```lua
print("Block size:", bb.length, "bytes")
```

#### `BasicBlock.index` -> `integer`

Index of this block within the function's block list

#### `BasicBlock.function` -> `Function`

Function containing this basic block

**Example:**
```lua
print("In function:", bb.function.name)
```

#### `BasicBlock.arch` -> `string`

Architecture name for this block

#### `BasicBlock.can_exit` -> `boolean`

Whether this block can cause the function to return

**Example:**
```lua
if bb.can_exit then print("Exit block") end
```

#### `BasicBlock.has_undetermined_outgoing_edges` -> `boolean`

Whether the block has unresolved indirect jumps

#### `BasicBlock.has_invalid_instructions` -> `boolean`

Whether the block contains instructions that failed to disassemble

#### `BasicBlock.is_il` -> `boolean`

Whether this is an IL basic block (vs native disassembly)

#### `BasicBlock.is_llil` -> `boolean`

Whether this is a Low Level IL basic block

#### `BasicBlock.is_mlil` -> `boolean`

Whether this is a Medium Level IL basic block

### Methods

#### `BasicBlock:instruction_count(...)` -> `integer`

Get the number of instructions in this block

**Example:**
```lua
print("Instructions:", bb:instruction_count())
```

#### `BasicBlock:outgoing_edges(...)` -> `table<{target: BasicBlock, type: string, back_edge: boolean}>`

Get edges to successor blocks

**Example:**
```lua
for _, edge in ipairs(bb:outgoing_edges()) do
    print("->", edge.target.start_addr, "type:", edge.type)
end
```

#### `BasicBlock:incoming_edges(...)` -> `table<{source: BasicBlock, type: string, back_edge: boolean}>`

Get edges from predecessor blocks

**Example:**
```lua
for _, edge in ipairs(bb:incoming_edges()) do
    print("<-", edge.source.start_addr)
end
```

#### `BasicBlock:dominators(...)` -> `table<BasicBlock>`

Get all blocks that dominate this block (must be executed before this block)

**Example:**
```lua
print("Dominated by", #bb:dominators(), "blocks")
```

#### `BasicBlock:strict_dominators(...)` -> `table<BasicBlock>`

Get dominators excluding the block itself

#### `BasicBlock:immediate_dominator(...)` -> `BasicBlock|nil`

Get the immediate dominator (closest dominating block)

**Example:**
```lua
local idom = bb:immediate_dominator()
if idom then print("Immediate dominator:", idom.start_addr) end
```

#### `BasicBlock:dominator_tree_children(...)` -> `table<BasicBlock>`

Get blocks that this block immediately dominates

#### `BasicBlock:dominance_frontier(...)` -> `table<BasicBlock>`

Get the dominance frontier (where dominance ends)

#### `BasicBlock:post_dominators(...)` -> `table<BasicBlock>`

Get all blocks that post-dominate this block (must be executed after this block)

#### `BasicBlock:strict_post_dominators(...)` -> `table<BasicBlock>`

Get post-dominators excluding the block itself

#### `BasicBlock:immediate_post_dominator(...)` -> `BasicBlock|nil`

Get the immediate post-dominator

#### `BasicBlock:post_dominator_tree_children(...)` -> `table<BasicBlock>`

Get blocks that this block immediately post-dominates

#### `BasicBlock:post_dominance_frontier(...)` -> `table<BasicBlock>`

Get the post-dominance frontier

#### `BasicBlock:dominates(...)` -> `boolean`

Check if this block dominates another block

**Example:**
```lua
if entry_block:dominates(bb) then
    print("Entry dominates this block")
end
```

#### `BasicBlock:strictly_dominates(...)` -> `boolean`

Check if this block strictly dominates another (excluding itself)

#### `BasicBlock:post_dominates(...)` -> `boolean`

Check if this block post-dominates another block

#### `BasicBlock:disassembly_text(...)` -> `table<string>`

Get raw disassembly text lines

#### `BasicBlock:disassembly(...)` -> `table<{addr: HexAddress, text: string, tokens: table}>`

Get structured disassembly with addresses and tokens

**Example:**
```lua
for _, line in ipairs(bb:disassembly()) do
    print(string.format("%s: %s", line.addr, line.text))
end
```

#### `BasicBlock:instructions(...)` -> `table<Instruction>`

Get all instructions in this block as Instruction objects

**Example:**
```lua
for _, instr in ipairs(bb:instructions()) do
    print(instr.address, instr.mnemonic, instr.text)
end
```

---

## Instruction

*Represents a single disassembled instruction. Provides access to the instruction's address, mnemonic, operands, raw bytes, and references.
*

### Properties

#### `Instruction.address` -> `HexAddress`

Address of the instruction

**Example:**
```lua
print("Instruction at:", instr.address)
```

#### `Instruction.mnemonic` -> `string`

Instruction mnemonic (e.g., "mov", "call", "jmp")

**Example:**
```lua
if instr.mnemonic == "call" then
    print("Found call instruction")
end
```

#### `Instruction.length` -> `integer`

Size of the instruction in bytes

**Example:**
```lua
print("Instruction is", instr.length, "bytes")
```

#### `Instruction.text` -> `string`

Full disassembly text including operands

**Example:**
```lua
print(instr.text)  -- e.g., "mov eax, [rbp-0x8]"
```

#### `Instruction.arch` -> `string`

Architecture of the instruction

### Methods

#### `Instruction:operands(...)` -> `table<string>`

Get the instruction's operands as strings

**Example:**
```lua
for i, op in ipairs(instr:operands()) do
    print("Operand", i, ":", op)
end
```

#### `Instruction:bytes(...)` -> `string`

Get the raw bytes of the instruction

**Example:**
```lua
local bytes = instr:bytes()
local hex = ""
for i = 1, #bytes do
    hex = hex .. string.format("%02x ", bytes:byte(i))
end
print("Bytes:", hex)
```

#### `Instruction:references(...)` -> `table<HexAddress>`

Get addresses referenced by this instruction

**Example:**
```lua
for _, ref in ipairs(instr:references()) do
    print("References:", ref)
end
```

---

## Variable

*Represents a variable within a function, including both parameters and local variables. Variables have names, types, and storage locations.
*

### Properties

#### `Variable.name` -> `string`

Name of the variable

**Example:**
```lua
print("Variable:", var.name)
```

#### `Variable.type` -> `string`

Type name of the variable (e.g., "int", "char*")

**Aliases:** `type_name`

**Example:**
```lua
print(var.name, ":", var.type)
```

#### `Variable.type_name` -> `string`

Alias for type property

#### `Variable.index` -> `integer`

Variable index within the function

#### `Variable.source_type` -> `string`

Source of the variable - "parameter", "local", or "unknown"

**Example:**
```lua
if var.source_type == "parameter" then
    print(var.name, "is a parameter")
end
```

### Methods

#### `Variable:location(...)` -> `table<{type: string, register: string|nil, offset: integer|nil}>`

Get the storage location of the variable

**Example:**
```lua
local loc = var:location()
if loc.register then
    print("In register:", loc.register)
elseif loc.offset then
    print("Stack offset:", loc.offset)
end
```

#### `Variable:set_name(...)` -> `boolean`

Rename the variable

**Parameters:**
- `name` (string) - New name for the variable

**Example:**
```lua
var:set_name("buffer_ptr")
```

#### `Variable:set_type(...)` -> `boolean`

Change the variable's type

**Parameters:**
- `typeStr` (string) - New type string (e.g., "uint32_t", "struct foo*")

**Example:**
```lua
var:set_type("char*")
```

---

## Llil

*Low Level IL (LLIL) function representation. LLIL is Binary Ninja's first level of intermediate representation, closely modeling the original assembly but with a consistent instruction set across architectures.
*

### Properties

#### `Llil.instruction_count` -> `integer`

Number of LLIL instructions in the function

**Example:**
```lua
print("LLIL instructions:", llil.instruction_count)
```

#### `Llil.basic_block_count` -> `integer`

Number of basic blocks in the LLIL

**Example:**
```lua
print("LLIL blocks:", llil.basic_block_count)
```

### Methods

#### `Llil:get_function(...)` -> `Function`

Get the native function this LLIL belongs to

**Example:**
```lua
local func = llil:get_function()
print("LLIL for:", func.name)
```

#### `Llil:instruction_at(...)` -> `LLILInstruction`

Get an LLIL instruction by index

**Parameters:**
- `index` (integer) - Instruction index (0-based)

**Example:**
```lua
for i = 0, llil.instruction_count - 1 do
    local instr = llil:instruction_at(i)
    print(i, llil:get_text(i))
end
```

#### `Llil:get_text(...)` -> `string`

Get the text representation of an LLIL instruction

**Parameters:**
- `index` (integer) - Instruction index (0-based)

**Example:**
```lua
print(llil:get_text(0))  -- First LLIL instruction
```

#### `Llil:create_graph(...)` -> `FlowGraph`

Create a flow graph for this LLIL function

**Example:**
```lua
local graph = llil:create_graph()
bv:show_graph_report("LLIL Graph", graph)
```

#### `Llil:create_graph_immediate(...)` -> `FlowGraph`

Create a flow graph and wait for layout to complete

---

## Mlil

*Medium Level IL (MLIL) function representation. MLIL lifts LLIL to a higher level, introducing typed variables, eliminating stack operations, and simplifying control flow.
*

### Properties

#### `Mlil.instruction_count` -> `integer`

Number of MLIL instructions in the function

**Example:**
```lua
print("MLIL instructions:", mlil.instruction_count)
```

#### `Mlil.basic_block_count` -> `integer`

Number of basic blocks in the MLIL

**Example:**
```lua
print("MLIL blocks:", mlil.basic_block_count)
```

### Methods

#### `Mlil:get_function(...)` -> `Function`

Get the native function this MLIL belongs to

#### `Mlil:instruction_at(...)` -> `MLILInstruction`

Get an MLIL instruction by index

**Parameters:**
- `index` (integer) - Instruction index (0-based)

#### `Mlil:get_text(...)` -> `string`

Get the text representation of an MLIL instruction

**Parameters:**
- `index` (integer) - Instruction index (0-based)

**Example:**
```lua
for i = 0, mlil.instruction_count - 1 do
    print(mlil:get_text(i))
end
```

#### `Mlil:create_graph(...)` -> `FlowGraph`

Create a flow graph for this MLIL function

**Example:**
```lua
local graph = mlil:create_graph()
bv:show_graph_report("MLIL Graph", graph)
```

#### `Mlil:create_graph_immediate(...)` -> `FlowGraph`

Create a flow graph and wait for layout to complete

---

## Hlil

*High Level IL (HLIL) function representation. HLIL is the highest level IR, featuring structured control flow (if/while/for), compound expressions, and a representation close to source code.
*

### Properties

#### `Hlil.instruction_count` -> `integer`

Number of HLIL statements in the function

**Example:**
```lua
print("HLIL statements:", hlil.instruction_count)
```

#### `Hlil.basic_block_count` -> `integer`

Number of basic blocks in the HLIL

**Example:**
```lua
print("HLIL blocks:", hlil.basic_block_count)
```

### Methods

#### `Hlil:get_function(...)` -> `Function`

Get the native function this HLIL belongs to

#### `Hlil:instruction_at(...)` -> `HLILInstruction`

Get an HLIL statement by index

**Parameters:**
- `index` (integer) - Statement index (0-based)

#### `Hlil:get_text(...)` -> `string`

Get the text representation of an HLIL statement

**Parameters:**
- `index` (integer) - Statement index (0-based)

**Example:**
```lua
print("HLIL decompilation:")
for i = 0, hlil.instruction_count - 1 do
    print(hlil:get_text(i))
end
```

#### `Hlil:create_graph(...)` -> `FlowGraph`

Create a flow graph for this HLIL function

**Example:**
```lua
local graph = hlil:create_graph()
bv:show_graph_report("HLIL Graph", graph)
```

#### `Hlil:create_graph_immediate(...)` -> `FlowGraph`

Create a flow graph and wait for layout to complete

---

## FlowGraph

*Represents a flow graph for visualizing control flow, call graphs, or custom graph structures. Can be displayed using bv:show_graph_report().
*

### Properties

#### `FlowGraph.width` -> `integer`

Width of the graph after layout (0 if layout not complete)

**Example:**
```lua
local graph = func:create_graph_immediate()
print("Graph width:", graph.width)
```

#### `FlowGraph.height` -> `integer`

Height of the graph after layout (0 if layout not complete)

#### `FlowGraph.node_count` -> `integer`

Number of nodes in the graph

**Example:**
```lua
print("Nodes:", graph.node_count)
```

#### `FlowGraph.has_nodes` -> `boolean`

Whether the graph has any nodes

#### `FlowGraph.function` -> `Function|nil`

Associated function (if this is a function graph)

#### `FlowGraph.view` -> `BinaryView|nil`

Associated BinaryView (if any)

#### `FlowGraph.is_il` -> `boolean`

Whether this is an IL graph (vs disassembly)

#### `FlowGraph.is_llil` -> `boolean`

Whether this is an LLIL graph

#### `FlowGraph.is_mlil` -> `boolean`

Whether this is an MLIL graph

#### `FlowGraph.is_hlil` -> `boolean`

Whether this is an HLIL graph

### Methods

#### `FlowGraph:nodes(...)` -> `table<FlowGraphNode>`

Get all nodes in the graph

**Example:**
```lua
for _, node in ipairs(graph:nodes()) do
    print("Node at:", node.x, node.y)
end
```

#### `FlowGraph:get_node(...)` -> `FlowGraphNode|nil`

Get a node by its index

**Parameters:**
- `index` (integer) - Node index (0-based)

#### `FlowGraph:add_node(...)`

Add a node to the graph

**Parameters:**
- `node` (FlowGraphNode) - Node to add

#### `FlowGraph:create_node(...)` -> `FlowGraphNode`

Create and add a new node to the graph

**Example:**
```lua
local node = graph:create_node()
node:set_lines({"Block A", "  instruction 1"})
```

#### `FlowGraph:clear_nodes(...)`

Remove all nodes from the graph

---

## FlowGraphNode

*Represents a single node in a FlowGraph. Contains display lines and edge connections to other nodes.
*

### Properties

#### `FlowGraphNode.x` -> `integer`

X position after layout

#### `FlowGraphNode.y` -> `integer`

Y position after layout

#### `FlowGraphNode.width` -> `integer`

Width of the node

#### `FlowGraphNode.height` -> `integer`

Height of the node

#### `FlowGraphNode.basic_block` -> `BasicBlock|nil`

Associated BasicBlock (if this node represents one)

#### `FlowGraphNode.highlight` -> `table`

Highlight color as {r, g, b, a} (0-255 values)

### Methods

#### `FlowGraphNode:lines(...)` -> `table<string>`

Get the text lines displayed in this node

#### `FlowGraphNode:set_lines(...)`

Set the text lines displayed in this node

**Parameters:**
- `lines` (table<string>) - Array of text lines to display

**Example:**
```lua
node:set_lines({"Block header", "  mov eax, 1", "  ret"})
```

#### `FlowGraphNode:outgoing_edges(...)` -> `table<{type: string, target: FlowGraphNode, back_edge: boolean}>`

Get all edges from this node to other nodes

**Example:**
```lua
for _, edge in ipairs(node:outgoing_edges()) do
    print("Edge type:", edge.type, "to node at", edge.target.x)
end
```

#### `FlowGraphNode:incoming_edges(...)` -> `table<{type: string, source: FlowGraphNode, back_edge: boolean}>`

Get all edges from other nodes to this node

#### `FlowGraphNode:add_outgoing_edge(...)`

Add an edge from this node to another node

**Parameters:**
- `type` (string) - Edge type - "UnconditionalBranch", "TrueBranch", "FalseBranch", "CallDestination", etc.
- `target` (FlowGraphNode) - Target node for the edge

**Example:**
```lua
node1:add_outgoing_edge("TrueBranch", node2)
node1:add_outgoing_edge("FalseBranch", node3)
```

---

## DataVariable

*Represents a data variable (global variable, constant, or data structure) defined in the binary.
*

### Properties

#### `DataVariable.address` -> `HexAddress`

Address where the data variable is located

**Example:**
```lua
print("Variable at:", var.address)
```

#### `DataVariable.type` -> `string`

Type of the data variable as a string (e.g., "int32_t", "char*")

**Example:**
```lua
print("Type:", var.type)
```

#### `DataVariable.name` -> `string`

Symbol name of the variable (empty string if no symbol)

**Example:**
```lua
if var.name ~= "" then
    print("Named:", var.name)
end
```

#### `DataVariable.auto_discovered` -> `boolean`

Whether this variable was auto-discovered during analysis (vs user-defined)

**Example:**
```lua
if var.auto_discovered then
    print("Auto-discovered variable")
end
```

#### `DataVariable.type_confidence` -> `integer`

Confidence level of the type assignment (0-255, higher is more confident)

---

## TagType

*Represents a type/category of tag that can be applied to addresses. Tag types define the name, icon, and visibility of tags.
*

### Properties

#### `TagType.id` -> `string`

Unique identifier for the tag type

#### `TagType.name` -> `string`

Display name of the tag type

**Example:**
```lua
print("Tag type:", tagType.name)
```

#### `TagType.icon` -> `string`

Icon/emoji displayed for this tag type

**Example:**
```lua
print("Icon:", tagType.icon)
```

#### `TagType.visible` -> `boolean`

Whether tags of this type are visible in the UI

#### `TagType.type` -> `string`

Category of tag type - "user", "notification", or "bookmarks"

### Methods

#### `TagType:set_name(...)`

Change the tag type's display name

**Parameters:**
- `name` (string) - New name for the tag type

#### `TagType:set_icon(...)`

Change the tag type's icon

**Parameters:**
- `icon` (string) - New icon (single character/emoji)

#### `TagType:set_visible(...)`

Set whether tags of this type are visible

**Parameters:**
- `visible` (boolean) - True to show, false to hide

---

## Tag

*Represents an individual tag instance applied at an address. Tags have a type (TagType) and optional data.
*

### Properties

#### `Tag.id` -> `string`

Unique identifier for this tag instance

#### `Tag.type` -> `TagType`

The TagType this tag belongs to

**Example:**
```lua
print("Tag category:", tag.type.name)
```

#### `Tag.data` -> `string`

Optional data/description stored with the tag

**Example:**
```lua
if tag.data ~= "" then
    print("Note:", tag.data)
end
```

### Methods

#### `Tag:set_data(...)`

Update the tag's data/description

**Parameters:**
- `data` (string) - New data string for the tag

**Example:**
```lua
tag:set_data("Updated note about this location")
```

---

## Type

*Represents a type in Binary Ninja's type system. Can represent primitives, pointers, arrays, structures, enumerations, and function types.
*

### Properties

#### `Type.type_class` -> `string`

Kind of type - "Void", "Bool", "Integer", "Float", "Structure", "Enumeration", "Pointer", "Array", "Function", etc.

**Example:**
```lua
if t.type_class == "Structure" then
    print("This is a struct")
end
```

#### `Type.type_class_value` -> `integer`

Numeric type class for programmatic comparisons

#### `Type.width` -> `integer`

Size of the type in bytes

**Example:**
```lua
print("Size:", t.width, "bytes")
```

#### `Type.alignment` -> `integer`

Alignment requirement in bytes

#### `Type.name` -> `string`

Name of the type (for named types)

#### `Type.is_void` -> `boolean`

Whether this is a void type

#### `Type.is_bool` -> `boolean`

Whether this is a boolean type

#### `Type.is_integer` -> `boolean`

Whether this is an integer type

#### `Type.is_float` -> `boolean`

Whether this is a floating-point type

#### `Type.is_structure` -> `boolean`

Whether this is a structure/class type

#### `Type.is_enumeration` -> `boolean`

Whether this is an enumeration type

#### `Type.is_pointer` -> `boolean`

Whether this is a pointer type

#### `Type.is_array` -> `boolean`

Whether this is an array type

#### `Type.is_function` -> `boolean`

Whether this is a function type

#### `Type.is_signed` -> `boolean`

Whether this is a signed integer type

#### `Type.is_const` -> `boolean`

Whether this type has const qualifier

#### `Type.is_volatile` -> `boolean`

Whether this type has volatile qualifier

#### `Type.element_count` -> `integer`

Number of elements (for array types, 0 otherwise)

#### `Type.return_type` -> `string`

Return type string (for function types)

#### `Type.has_variable_arguments` -> `boolean`

Whether function accepts variable arguments (for function types)

#### `Type.can_return` -> `boolean`

Whether function can return (false for noreturn, for function types)

### Methods

#### `Type:target(...)` -> `Type|nil`

Get the target type for pointer types

**Example:**
```lua
if t.is_pointer then
    local target = t:target()
    if target then print("Points to:", target:get_string()) end
end
```

#### `Type:members(...)` -> `table<{name: string, offset: integer, type: string, width: integer, access: string}>|nil`

Get structure members (for structure types only)

**Example:**
```lua
if t.is_structure then
    for _, m in ipairs(t:members()) do
        print(string.format("+0x%x %s: %s", m.offset, m.name, m.type))
    end
end
```

#### `Type:get_member_by_name(...)` -> `table<{name, offset, type, width, access}>|nil`

Get a structure member by name

**Parameters:**
- `name` (string) - Member name to look up

#### `Type:get_member_at_offset(...)` -> `table<{name, offset, type, width, access}>|nil`

Get a structure member at a specific offset

**Parameters:**
- `offset` (integer) - Byte offset within the structure

#### `Type:enum_members(...)` -> `table<{name: string, value: integer, is_default: boolean}>|nil`

Get enumeration members (for enum types only)

**Example:**
```lua
if t.is_enumeration then
    for _, m in ipairs(t:enum_members()) do
        print(m.name, "=", m.value)
    end
end
```

#### `Type:parameters(...)` -> `table<{name: string, type: string, has_default: boolean}>|nil`

Get function parameters (for function types only)

**Example:**
```lua
if t.is_function then
    print("Parameters:")
    for i, p in ipairs(t:parameters()) do
        print(i, p.name, p.type)
    end
end
```

#### `Type:get_string(...)` -> `string`

Get the full type declaration as a string

**Example:**
```lua
print("Full type:", t:get_string())
```

---

## GlobalFunctions

*Global utility functions available without a receiver object.
*

---

## Magic Variables

These variables are automatically available in the Lua scripting environment:

| Variable | Type | Description |
|----------|------|-------------|
| `bv` | BinaryView | Current binary view instance |
| `current_function` | Function | Currently selected function (may be nil) |
| `current_address` | HexAddress | Current address in the UI |
| `here` | HexAddress | Alias for current_address |
| `current_selection` | Selection | Currently selected range |
