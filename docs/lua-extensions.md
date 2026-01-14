# Binary Ninja Lua API Extensions

*Auto-generated from Lua extension source code*

**Extension Modules:** 3 | **Extension Methods:** 55

## Overview

This document covers the optional Lua API extensions that provide idiomatic
Lua patterns for Binary Ninja analysis. These extensions are automatically
loaded when available and provide enhanced functionality beyond the core C++ bindings.

## Table of Contents

- [Collections](#collections)
- [Fluent](#fluent)
- [Utils](#utils)

---

## Collections

*Idiomatic Lua collection APIs and iterators for Binary Ninja This module provides idiomatic Lua collection APIs and iterator patterns for working with Binary Ninja objects. It extends BinaryView and Function objects with convenient iterator methods and fluent collection wrappers.*

**Source:** `lua-api/collections.lua`

### Methods

#### `BinaryView:each_function()` → `function`

Iterate over all functions in the binary

**Returns:**
`function` - Iterator function that yields Function objects

**Example:**
```lua
for func in bv:each_function() do
    print(func.name, func.size)
end
```

#### `BinaryView:each_section()` → `function`

Iterate over all sections in the binary

**Returns:**
`function` - Iterator function that yields Section objects

**Example:**
```lua
for section in bv:each_section() do
    print(section.name, section.start_addr)
end
```

#### `BinaryView:functions_collection()` → `Collection`

Get a fluent collection wrapper for all functions

**Returns:**
`Collection` - Collection of Function objects

**Example:**
```lua
local large_funcs = bv:functions_collection()
    :filter(function(f) return f.size > 1000 end)
    :sort(function(a, b) return a.size > b.size end)
```

#### `BinaryView:sections_collection()` → `Collection`

Get a fluent collection wrapper for all sections

**Returns:**
`Collection` - Collection of Section objects

#### `BinaryView:each_tag()` → `function`

Iterate over all tags in the binary

**Returns:**
`function` - Iterator function that yields Tag objects

**Example:**
```lua
for tag in bv:each_tag() do
    print(tag.type.name, tag.data)
end
```

#### `BinaryView:tags_collection()` → `Collection`

Get a fluent collection wrapper for all tags

**Returns:**
`Collection` - Collection of Tag objects

**Example:**
```lua
local important_tags = bv:tags_collection()
    :filter(function(t) return t.type.name == "Important" end)
```

#### `BinaryView:types_collection()` → `Collection`

Get a fluent collection wrapper for all defined types

**Returns:**
`Collection` - Collection of {name, type} entries

**Example:**
```lua
local structs = bv:types_collection()
    :filter(function(t) return t.type.is_structure end)
```

#### `BinaryView:each_type()` → `function`

Iterate over all defined types in the binary

**Returns:**
`function` - Iterator function that yields {name, type} entries

**Example:**
```lua
for entry in bv:each_type() do
    print(entry.name, entry.type.type_class)
end
```

#### `BinaryView:each_data_var()` → `function`

Iterate over all data variables in the binary

**Returns:**
`function` - Iterator function that yields DataVariable objects

**Example:**
```lua
for dv in bv:each_data_var() do
    print(string.format("0x%x: %s", dv.address.value, dv.type))
end
```

#### `BinaryView:data_vars_collection()` → `Collection`

Get a fluent collection wrapper for all data variables

**Returns:**
`Collection` - Collection of DataVariable objects

#### `Function:each_basic_block()` → `function`

Iterate over all basic blocks in the function

**Returns:**
`function` - Iterator function that yields BasicBlock objects

**Example:**
```lua
for block in func:each_basic_block() do
    print(string.format("Block at 0x%x", block.start_addr))
end
```

#### `Function:each_caller()` → `function`

Iterate over all functions that call this function

**Returns:**
`function` - Iterator function that yields Function objects

**Example:**
```lua
for caller in func:each_caller() do
    print("Called by:", caller.name)
end
```

#### `Function:each_call()` → `function`

Iterate over all functions called by this function

**Returns:**
`function` - Iterator function that yields Function objects

**Example:**
```lua
for called_func in func:each_call() do
    print("Calls:", called_func.name)
end
```

#### `Function:basic_blocks_collection()` → `Collection`

Get a fluent collection wrapper for basic blocks

**Returns:**
`Collection` - Collection of BasicBlock objects

#### `Function:callers_collection()` → `Collection`

Get a fluent collection wrapper for caller functions

**Returns:**
`Collection` - Collection of Function objects

#### `Function:calls_collection()` → `Collection`

Get a fluent collection wrapper for called functions

**Returns:**
`Collection` - Collection of Function objects

#### `Function:each_variable()` → `function`

Iterate over all variables in the function

**Returns:**
`function` - Iterator function that yields Variable objects

**Example:**
```lua
for var in func:each_variable() do
    print(var.name, var.type)
end
```

#### `Function:variables_collection()` → `Collection`

Get a fluent collection wrapper for all variables

**Returns:**
`Collection` - Collection of Variable objects

**Example:**
```lua
local stack_vars = func:variables_collection()
    :filter(function(v) return v.source_type == "stack" end)
```

#### `Function:each_clobbered_reg()` → `function`

Iterate over registers clobbered by this function

**Returns:**
`function` - Iterator function that yields register names

**Example:**
```lua
for reg in func:each_clobbered_reg() do
    print("Clobbers:", reg)
end
```

#### `Function:each_tag()` → `function`

Iterate over all tags attached to this function

**Returns:**
`function` - Iterator function that yields Tag objects

**Example:**
```lua
for tag in func:each_tag() do
    print(tag.type.name, tag.data)
end
```

#### `Function:tags_collection()` → `Collection`

Get a fluent collection wrapper for function tags

**Returns:**
`Collection` - Collection of Tag objects

---

## Fluent

*Advanced fluent API patterns for Binary Ninja analysis This module provides advanced fluent API patterns for complex Binary Ninja analysis workflows. It includes query builders and analysis helpers that allow for chainable, readable analysis code.*

**Source:** `lua-api/fluent.lua`

### Methods

#### `Query:new(bv)` → `Query`

Create a new Query instance

**Parameters:**
- `bv` (BinaryView) - The binary view to query

**Returns:**
`Query` - New query instance

#### `Query:functions()` → `Query`

Start a query with all functions in the binary

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local all_functions = bv:query():functions():get()
```

#### `Query:at_address(addr)` → `Query`

Filter to only the function at the specified address

**Parameters:**
- `addr` (integer) - Address to find function at

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local func_at_addr = bv:query():at_address(0x10001000):get()
```

#### `Query:with_name(pattern)` → `Query`

Filter functions by name pattern (Lua pattern matching)

**Parameters:**
- `pattern` (string) - Lua pattern to match against function names

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local main_funcs = bv:query():functions():with_name("main.*"):get()
```

#### `Query:larger_than(size)` → `Query`

Filter functions larger than specified size in bytes

**Parameters:**
- `size` (integer) - Minimum size in bytes

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local large_funcs = bv:query():functions():larger_than(1000):get()
```

#### `Query:with_calls_to(target_pattern)` → `Query`

Filter functions that call functions matching the pattern

**Parameters:**
- `target_pattern` (string) - Pattern to match against called function names

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local malloc_callers = bv:query():functions():with_calls_to("malloc"):get()
```

#### `Query:exported_only()` → `Query`

Filter to only exported functions

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local exports = bv:query():functions():exported_only():get()
```

#### `Query:auto_discovered()` → `Query`

Filter to only auto-discovered (not user-defined) functions

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local auto_funcs = bv:query():functions():auto_discovered():get()
```

#### `Query:user_defined()` → `Query`

Filter to only user-defined (not auto-discovered) functions

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local user_funcs = bv:query():functions():user_defined():get()
```

#### `Query:thunks_only()` → `Query`

Filter to only thunk functions

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local thunks = bv:query():functions():thunks_only():get()
```

#### `Query:exclude_thunks()` → `Query`

Filter out thunk functions

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local real_funcs = bv:query():functions():exclude_thunks():get()
```

#### `Query:pure_only()` → `Query`

Filter to only pure functions (no side effects)

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local pure_funcs = bv:query():functions():pure_only():get()
```

#### `Query:with_stack_adjustment()` → `Query`

Filter to functions with non-zero stack adjustment

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local stack_funcs = bv:query():functions():with_stack_adjustment():get()
```

#### `Query:with_variables()` → `Query`

Filter to functions that have variables

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local funcs_with_vars = bv:query():functions():with_variables():get()
```

#### `Query:with_tags()` → `Query`

Filter to functions that have tags attached

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local tagged_funcs = bv:query():functions():with_tags():get()
```

#### `Query:sort_by(key_func, descending)` → `Query`

Sort results by a key function

**Parameters:**
- `key_func` (function) - Function that takes an item and returns a sortable value
- `descending` (boolean) - Optional: true for descending order, false/nil for ascending

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local sorted_by_size = bv:query():functions():sort_by(function(f) return f.size end, true):get()
```

#### `Query:limit(count)` → `Query`

Limit results to first N items

**Parameters:**
- `count` (integer) - Maximum number of items to return

**Returns:**
`Query` - Self for method chaining

**Example:**
```lua
local top_10 = bv:query():functions():sort_by(function(f) return f.size end, true):limit(10):get()
```

#### `Query:execute()` → `table`

Execute the query and return results

**Returns:**
`table` - Array of results from the query

#### `Query:get()` → `table`

Convenience method to execute the query (alias for execute)

**Returns:**
`table` - Array of results from the query

**Example:**
```lua
local results = bv:query():functions():larger_than(100):get()
```

#### `Analysis:function_classification()` → `table`

Classify all functions by type (thunk, exported, auto, pure, etc.)

**Returns:**
`table` - Statistics about function classifications

**Example:**
```lua
local class = bv:analyze():function_classification()
print("Exported:", class.exported)
print("Thunks:", class.thunks)
print("Auto-discovered:", class.auto_discovered)
```

#### `Analysis:tag_report()` → `table`

Generate statistics about tags in the binary

**Returns:**
`table` - Statistics about tags and tag types

**Example:**
```lua
local tags = bv:analyze():tag_report()
print("Total tags:", tags.total_tags)
for type_name, count in pairs(tags.by_type) do
print(type_name, count)
end
```

#### `Analysis:stack_analysis()` → `table`

Analyze stack usage across all functions

**Returns:**
`table` - Statistics about stack adjustments

**Example:**
```lua
local stacks = bv:analyze():stack_analysis()
print("Max stack:", stacks.max_adjustment)
print("Average:", stacks.average_adjustment)
```

#### `Analysis:variable_analysis()` → `table`

Analyze variable usage across all functions

**Returns:**
`table` - Statistics about variables

**Example:**
```lua
local vars = bv:analyze():variable_analysis()
print("Total variables:", vars.total_variables)
print("Functions with vars:", vars.functions_with_variables)
```

#### `Analysis:type_analysis()` → `table`

Analyze defined types in the binary

**Returns:**
`table` - Statistics about types

**Example:**
```lua
local types = bv:analyze():type_analysis()
print("Structures:", types.structures)
print("Enumerations:", types.enumerations)
```

#### `BinaryView:query()` → `Query`

Create a fluent query builder for complex analysis workflows

**Returns:**
`Query` - Query builder instance for chaining operations

**Example:**
```lua
local large_functions = bv:query()
    :functions()
    :larger_than(1000)
    :with_calls_to("malloc")
    :sort_by(function(f) return f.size end, true)
    :limit(10)
    :get()
```

#### `BinaryView:analyze()` → `Analysis`

Create an analysis helper for common analysis patterns

**Returns:**
`Analysis` - Analysis helper instance with pre-built workflows

**Example:**
```lua
local connectivity = bv:analyze():connectivity_report()
local size_stats = bv:analyze():size_analysis()
```

---

## Utils

*Enhanced utility functions for debugging and data inspection Binary Ninja-specific utility functions for reverse engineering tasks. This module provides helper functions that build on the core Binary Ninja API to offer commonly needed functionality for analysis scripts and plugins, including the powerful dump() function with flexible formatting options.*

**Source:** `lua-api/utils.lua`

### Methods

#### `utils.find_functions_by_name_pattern(bv, pattern)` → `table`

Find all functions whose names match a Lua pattern

**Parameters:**
- `bv` (BinaryView) - The binary view to search
- `pattern` (string) - Lua pattern to match against function names

**Returns:**
`table` - Array of Function objects matching the pattern

**Example:**
```lua
local crypto_funcs = utils.find_functions_by_name_pattern(bv, ".*crypt.*")
for _, func in ipairs(crypto_funcs) do
print("Found crypto function:", func.name)
end
```

#### `utils.find_functions_by_size_range(bv, min_size, max_size)` → `table`

Find functions within a specific size range

**Parameters:**
- `bv` (BinaryView) - The binary view to search
- `min_size` (integer) - Minimum function size in bytes
- `max_size` (integer) - Maximum function size in bytes (optional)

**Returns:**
`table` - Array of Function objects within the size range

**Example:**
```lua
local small_funcs = utils.find_functions_by_size_range(bv, 1, 50)
local large_funcs = utils.find_functions_by_size_range(bv, 1000)
```

#### `utils.get_call_depth(func, visited)` → `integer`

Calculate the maximum call depth from a function (recursive calls)

**Parameters:**
- `func` (Function) - The function to analyze
- `visited` (table) - Internal parameter for recursion tracking

**Returns:**
`integer` - Maximum call depth from this function

**Example:**
```lua
local depth = utils.get_call_depth(main_func)
print(string.format("Max call depth from %s: %d", main_func.name, depth))
```

#### `utils.find_strings_in_function(func, pattern)` → `table`

Find string references within a function that match a pattern

**Parameters:**
- `func` (Function) - The function to search
- `pattern` (string) - Lua pattern to match against string contents

**Returns:**
`table` - Array of {address, string} tables for matching strings

**Example:**
```lua
local api_strings = utils.find_strings_in_function(func, ".*API.*")
for _, str_ref in ipairs(api_strings) do
print(string.format("API string at 0x%x: %s", str_ref.address, str_ref.string))
end
```

#### `utils.get_function_complexity(func)` → `table`

Calculate basic complexity metrics for a function

**Parameters:**
- `func` (Function) - The function to analyze

**Returns:**
`table` - Table with complexity metrics: {blocks, edges, cyclomatic}

**Example:**
```lua
local complexity = utils.get_function_complexity(func)
print(string.format("Function %s: %d blocks, cyclomatic complexity: %d",
func.name, complexity.blocks, complexity.cyclomatic))
```

#### `utils.get_imports_used_by_function(func)` → `table`

Get all imported functions referenced by a specific function

**Parameters:**
- `func` (Function) - The function to analyze

**Returns:**
`table` - Array of import symbol names used by this function

**Example:**
```lua
local imports = utils.get_imports_used_by_function(func)
for _, import_name in ipairs(imports) do
print("Function uses import:", import_name)
end
```

#### `utils.init()` 

Initialize the utilities module and extend Binary Ninja objects

**Example:**
```lua
local utils = require('utils')
utils.init()
```

#### `dump(value, config, indent)` → `string`

Recursively format any Lua value with clean, readable output

**Parameters:**
- `value` (any) - The value to format (table, number, string, userdata, etc.)
- `config` (table) - Optional configuration: {compact=bool, show_indices=bool}
- `indent` (number) - Internal parameter for indentation level

**Returns:**
`string` - Formatted string representation of the value

**Example:**
```lua
local data = {name="test", values={1,2,3}, nested={a=1}}
print(dump(data))
-- Compact format
print(dump(data, {compact=true}))
-- Show array indices
print(dump({func1, func2}, {show_indices=true}))
```

---
