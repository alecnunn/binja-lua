--[[
@file utils.lua
@brief Enhanced utility functions for debugging and data inspection

Binary Ninja-specific utility functions for reverse engineering tasks.
This module provides helper functions that build on the core Binary Ninja API to offer
commonly needed functionality for analysis scripts and plugins, including the powerful
dump() function with flexible formatting options.

@example
local utils = require('utils')
utils.init()

-- Enhanced dump function with configuration
dump(data, {compact=true, show_indices=true})

-- Find functions by pattern
local funcs = utils.find_functions_by_name_pattern(bv, ".*crypt.*")

-- Get function call graph depth
local depth = utils.get_call_depth(func)
]]

local utils = {}

--[[
@luaapi utils.find_functions_by_name_pattern(bv, pattern)
@description Find all functions whose names match a Lua pattern
@param bv BinaryView The binary view to search
@param pattern string Lua pattern to match against function names
@return table Array of Function objects matching the pattern
@example
local crypto_funcs = utils.find_functions_by_name_pattern(bv, ".*crypt.*")
for _, func in ipairs(crypto_funcs) do
    print("Found crypto function:", func.name)
end
]]
function utils.find_functions_by_name_pattern(bv, pattern)
    local matching_funcs = {}
    for func in bv:each_function() do
        local name = func.name
        if name and name:match(pattern) then
            table.insert(matching_funcs, func)
        end
    end
    return matching_funcs
end

--[[
@luaapi utils.find_functions_by_size_range(bv, min_size, max_size)
@description Find functions within a specific size range
@param bv BinaryView The binary view to search
@param min_size integer Minimum function size in bytes
@param max_size integer Maximum function size in bytes (optional)
@return table Array of Function objects within the size range
@example
local small_funcs = utils.find_functions_by_size_range(bv, 1, 50)
local large_funcs = utils.find_functions_by_size_range(bv, 1000)
]]
function utils.find_functions_by_size_range(bv, min_size, max_size)
    local matching_funcs = {}
    for func in bv:each_function() do
        local size = func.size
        if size >= min_size and (not max_size or size <= max_size) then
            table.insert(matching_funcs, func)
        end
    end
    return matching_funcs
end


--[[
@luaapi utils.get_call_depth(func, visited)
@description Calculate the maximum call depth from a function (recursive calls)
@param func Function The function to analyze
@param visited table Internal parameter for recursion tracking
@return integer Maximum call depth from this function
@example
local depth = utils.get_call_depth(main_func)
print(string.format("Max call depth from %s: %d", main_func.name, depth))
]]
function utils.get_call_depth(func, visited)
    visited = visited or {}
    local func_addr = func.start_addr.value

    -- Prevent infinite recursion
    if visited[func_addr] then
        return 0
    end
    visited[func_addr] = true

    local max_depth = 0
    for callee in func:each_call() do
        local depth = utils.get_call_depth(callee, visited)
        max_depth = math.max(max_depth, depth + 1)
    end

    visited[func_addr] = nil
    return max_depth
end

--[[
@luaapi utils.find_strings_in_function(func, pattern)
@description Find string references within a function that match a pattern
@param func Function The function to search
@param pattern string Lua pattern to match against string contents
@return table Array of {address, string} tables for matching strings
@example
local api_strings = utils.find_strings_in_function(func, ".*API.*")
for _, str_ref in ipairs(api_strings) do
    print(string.format("API string at 0x%x: %s", str_ref.address, str_ref.string))
end
]]
function utils.find_strings_in_function(func, pattern)
    local matching_strings = {}
    local bv = func.view

    -- Get all string references in the function
    for block in func:each_basic_block() do
        local start_addr = block.start_addr
        local end_addr = block.end_addr

        -- Iterate through instructions in the block
        local addr = start_addr
        while addr < end_addr do
            local refs = bv:get_code_refs(addr)
            if refs then
                for _, ref in ipairs(refs) do
                    local str = bv:get_string_at(ref)
                    if str and str.value and str.value:match(pattern) then
                        table.insert(matching_strings, {
                            address = ref,
                            string = str.value
                        })
                    end
                end
            end

            local instr = bv:get_disassembly(addr)
            if instr and instr.length then
                addr = addr + instr.length
            else
                break
            end
        end
    end

    return matching_strings
end

--[[
@luaapi utils.get_function_complexity(func)
@description Calculate basic complexity metrics for a function
@param func Function The function to analyze
@return table Table with complexity metrics: {blocks, edges, cyclomatic}
@example
local complexity = utils.get_function_complexity(func)
print(string.format("Function %s: %d blocks, cyclomatic complexity: %d",
    func.name, complexity.blocks, complexity.cyclomatic))
]]
function utils.get_function_complexity(func)
    local block_count = 0
    local edge_count = 0

    for block in func:each_basic_block() do
        block_count = block_count + 1
        local outgoing = block:outgoing_edges()
        if outgoing then
            edge_count = edge_count + #outgoing
        end
    end

    -- Cyclomatic complexity: E - N + 2P (P=1 for single function)
    local cyclomatic = edge_count - block_count + 2

    return {
        blocks = block_count,
        edges = edge_count,
        cyclomatic = cyclomatic
    }
end

--[[
@luaapi utils.get_imports_used_by_function(func)
@description Get all imported functions referenced by a specific function
@param func Function The function to analyze
@return table Array of import symbol names used by this function
@example
local imports = utils.get_imports_used_by_function(func)
for _, import_name in ipairs(imports) do
    print("Function uses import:", import_name)
end
]]
function utils.get_imports_used_by_function(func)
    local imports = {}
    local bv = func.view

    for block in func:each_basic_block() do
        local addr = block.start_addr
        while addr < block.end_addr do
            local refs = bv:get_code_refs_from(addr)
            if refs then
                for _, ref in ipairs(refs) do
                    local symbol = bv:get_symbol_at(ref)
                    if symbol and symbol.type == "ImportedFunction" then
                        table.insert(imports, symbol.name)
                    end
                end
            end

            local instr = bv:get_disassembly(addr)
            if instr and instr.length then
                addr = addr + instr.length
            else
                break
            end
        end
    end

    return imports
end


--[[
@luaapi utils.init()
@description Initialize the utilities module and extend Binary Ninja objects
@example
local utils = require('utils')
utils.init()
]]
function utils.init()
    -- Extend BinaryView with utility methods
    local bv_mt = debug.getregistry()["BinaryNinja.BinaryView"]
    if bv_mt then
        bv_mt.__index.find_functions_by_pattern = function(self, pattern)
            return utils.find_functions_by_name_pattern(self, pattern)
        end

        bv_mt.__index.find_functions_by_size = function(self, min_size, max_size)
            return utils.find_functions_by_size_range(self, min_size, max_size)
        end

    end

    -- Extend Function with utility methods
    local func_mt = debug.getregistry()["BinaryNinja.Function"]
    if func_mt then
        func_mt.__index.get_call_depth = function(self)
            return utils.get_call_depth(self)
        end

        func_mt.__index.get_complexity = function(self)
            return utils.get_function_complexity(self)
        end

        func_mt.__index.find_strings = function(self, pattern)
            return utils.find_strings_in_function(self, pattern)
        end

        func_mt.__index.get_imports = function(self)
            return utils.get_imports_used_by_function(self)
        end
    end
end

--[[
@luaapi dump(value, config, indent)
@description Recursively format any Lua value with clean, readable output
@param value any The value to format (table, number, string, userdata, etc.)
@param config table Optional configuration: {compact=bool, show_indices=bool}
@param indent number Internal parameter for indentation level
@return string Formatted string representation of the value
@example
local data = {name="test", values={1,2,3}, nested={a=1}}
print(dump(data))
-- Compact format
print(dump(data, {compact=true}))
-- Show array indices
print(dump({func1, func2}, {show_indices=true}))
]]
function dump(o, config, indent)
    -- Handle backward compatibility - if config is a number, it's the old indent parameter
    if type(config) == "number" then
        indent = config
        config = {}
    end

    config = config or {}
    indent = indent or 0
    local indent_str = string.rep("  ", indent)
    local next_indent_str = string.rep("  ", indent + 1)

    if type(o) == 'table' then
        local result = {}
        local is_array = true
        local max_index = 0

        -- Check if this is an array-like table
        for k, v in pairs(o) do
            if type(k) == 'number' and k > 0 and k == math.floor(k) then
                max_index = math.max(max_index, k)
            else
                is_array = false
                break
            end
        end

        -- For arrays, check if indices are consecutive starting from 1
        if is_array then
            for i = 1, max_index do
                if o[i] == nil then
                    is_array = false
                    break
                end
            end
        end

        -- Force show_indices to override array formatting if requested
        local force_indices = config.show_indices and is_array

        table.insert(result, "{")

        if is_array and not force_indices then
            -- Format as array (without explicit indices)
            if config.compact then
                local values = {}
                for i = 1, max_index do
                    table.insert(values, dump(o[i], config, 0))
                end
                return "{" .. table.concat(values, ", ") .. "}"
            else
                for i = 1, max_index do
                    local value_str = dump(o[i], config, indent + 1)
                    if i < max_index then
                        table.insert(result, next_indent_str .. value_str .. ",")
                    else
                        table.insert(result, next_indent_str .. value_str)
                    end
                end
            end
        else
            -- Format as object with explicit keys (including forced array indices)
            local keys = {}
            if force_indices then
                -- For arrays with show_indices, use numeric keys
                for i = 1, max_index do
                    table.insert(keys, i)
                end
            else
                -- For objects, collect all keys
                for k in pairs(o) do
                    table.insert(keys, k)
                end
            end

            table.sort(keys, function(a, b)
                -- Sort strings before numbers, then alphabetically/numerically
                if type(a) == type(b) then
                    if type(a) == 'number' then
                        return a < b  -- Numeric sort for numbers
                    else
                        return tostring(a) < tostring(b)  -- String sort for non-numbers
                    end
                else
                    return type(a) == 'string'
                end
            end)

            if config.compact then
                local pairs_strs = {}
                for i, k in ipairs(keys) do
                    local key_str
                    if type(k) == 'string' and k:match('^[%a_][%w_]*$') then
                        key_str = k
                    else
                        key_str = '[' .. dump(k, config, 0) .. ']'
                    end
                    local value_str = dump(o[k], config, 0)
                    table.insert(pairs_strs, key_str .. " = " .. value_str)
                end
                return "{" .. table.concat(pairs_strs, ", ") .. "}"
            else
                for i, k in ipairs(keys) do
                    local key_str
                    if type(k) == 'string' and k:match('^[%a_][%w_]*$') then
                        -- Use clean syntax for valid identifiers
                        key_str = k
                    else
                        -- Use bracket notation for complex keys
                        key_str = '[' .. dump(k, config, 0) .. ']'
                    end

                    local value_str = dump(o[k], config, indent + 1)
                    if i < #keys then
                        table.insert(result, next_indent_str .. key_str .. " = " .. value_str .. ",")
                    else
                        table.insert(result, next_indent_str .. key_str .. " = " .. value_str)
                    end
                end
            end
        end

        if not config.compact then
            table.insert(result, indent_str .. "}")
            return table.concat(result, "\n")
        end

    elseif type(o) == 'string' then
        return '"' .. o:gsub('\\', '\\\\'):gsub('"', '\\"'):gsub('\n', '\\n'):gsub('\t', '\\t') .. '"'
    elseif type(o) == 'number' then
        -- Format large numbers as hex if they look like addresses
        if o > 0x1000 and o == math.floor(o) then
            return string.format("0x%x", o)
        else
            return tostring(o)
        end
    elseif type(o) == 'boolean' then
        return tostring(o)
    elseif type(o) == 'nil' then
        return 'nil'
    else
        -- For userdata, functions, etc., use tostring
        return tostring(o)
    end
end

return utils