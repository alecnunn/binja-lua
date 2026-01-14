--[[
@file fluent.lua
@brief Advanced fluent API patterns for Binary Ninja analysis

This module provides advanced fluent API patterns for complex Binary Ninja
analysis workflows. It includes query builders and analysis helpers that
allow for chainable, readable analysis code.

@example
-- Find large functions that call specific APIs
local results = bv:query()
    :functions()
    :larger_than(1000)
    :with_calls_to("malloc")
    :sort_by(function(f) return f.size end, true)
    :limit(10)
    :get()

-- Perform common analysis patterns
local connectivity = bv:analyze():connectivity_report()
local size_stats = bv:analyze():size_analysis()
]]

local fluent = {}

--[[
@class Query
@brief Fluent query builder for complex analysis workflows

The Query class provides a chainable interface for building complex
analysis queries on Binary Ninja objects. Each method returns the query
object, allowing for method chaining.

@example
local large_functions = bv:query()
    :functions()
    :larger_than(1000)
    :with_min_callers(5)
    :get()
]]
local Query = {}
Query.__index = Query

--[[
@luaapi Query:new(bv)
@description Create a new Query instance
@param bv BinaryView The binary view to query
@return Query New query instance
]]
function Query:new(bv)
    local obj = {
        bv = bv,
        steps = {}
    }
    setmetatable(obj, Query)
    return obj
end

--[[
@luaapi Query:functions()
@description Start a query with all functions in the binary
@return Query Self for method chaining
@example
local all_functions = bv:query():functions():get()
]]
function Query:functions()
    table.insert(self.steps, function(data)
        return data.bv:functions()
    end)
    return self
end

--[[
@luaapi Query:at_address(addr)
@description Filter to only the function at the specified address
@param addr integer Address to find function at
@return Query Self for method chaining
@example
local func_at_addr = bv:query():at_address(0x10001000):get()
]]
function Query:at_address(addr)
    table.insert(self.steps, function(data)
        local func = data.bv:get_function_at(addr)
        return func and {func} or {}
    end)
    return self
end

--[[
@luaapi Query:with_name(pattern)
@description Filter functions by name pattern (Lua pattern matching)
@param pattern string Lua pattern to match against function names
@return Query Self for method chaining
@example
local main_funcs = bv:query():functions():with_name("main.*"):get()
]]
function Query:with_name(pattern)
    table.insert(self.steps, function(data)
        local results = {}
        for _, item in ipairs(data) do
            if string.match(item.name, pattern) then
                table.insert(results, item)
            end
        end
        return results
    end)
    return self
end

--[[
@luaapi Query:larger_than(size)
@description Filter functions larger than specified size in bytes
@param size integer Minimum size in bytes
@return Query Self for method chaining
@example
local large_funcs = bv:query():functions():larger_than(1000):get()
]]
function Query:larger_than(size)
    table.insert(self.steps, function(data)
        local results = {}
        for _, item in ipairs(data) do
            if item.size > size then
                table.insert(results, item)
            end
        end
        return results
    end)
    return self
end

--[[
@luaapi Query:with_calls_to(target_pattern)
@description Filter functions that call functions matching the pattern
@param target_pattern string Pattern to match against called function names
@return Query Self for method chaining
@example
local malloc_callers = bv:query():functions():with_calls_to("malloc"):get()
]]
function Query:with_calls_to(target_pattern)
    table.insert(self.steps, function(data)
        local results = {}
        for _, func in ipairs(data) do
            local calls = func:calls() or {}
            for _, called_func in ipairs(calls) do
                if string.match(called_func.name, target_pattern) then
                    table.insert(results, func)
                    break
                end
            end
        end
        return results
    end)
    return self
end

function Query:with_min_callers(count)
    table.insert(self.steps, function(data)
        local results = {}
        for _, item in ipairs(data) do
            local callers = item:callers() or {}
            if #callers >= count then
                table.insert(results, item)
            end
        end
        return results
    end)
    return self
end

function Query:with_min_blocks(count)
    table.insert(self.steps, function(data)
        local results = {}
        for _, item in ipairs(data) do
            local blocks = item:basic_blocks() or {}
            if #blocks >= count then
                table.insert(results, item)
            end
        end
        return results
    end)
    return self
end

--[[
@luaapi Query:exported_only()
@description Filter to only exported functions
@return Query Self for method chaining
@example
local exports = bv:query():functions():exported_only():get()
]]
function Query:exported_only()
    table.insert(self.steps, function(data)
        local results = {}
        for _, item in ipairs(data) do
            if item.is_exported then
                table.insert(results, item)
            end
        end
        return results
    end)
    return self
end

--[[
@luaapi Query:auto_discovered()
@description Filter to only auto-discovered (not user-defined) functions
@return Query Self for method chaining
@example
local auto_funcs = bv:query():functions():auto_discovered():get()
]]
function Query:auto_discovered()
    table.insert(self.steps, function(data)
        local results = {}
        for _, item in ipairs(data) do
            if item.auto then
                table.insert(results, item)
            end
        end
        return results
    end)
    return self
end

--[[
@luaapi Query:user_defined()
@description Filter to only user-defined (not auto-discovered) functions
@return Query Self for method chaining
@example
local user_funcs = bv:query():functions():user_defined():get()
]]
function Query:user_defined()
    table.insert(self.steps, function(data)
        local results = {}
        for _, item in ipairs(data) do
            if not item.auto then
                table.insert(results, item)
            end
        end
        return results
    end)
    return self
end

--[[
@luaapi Query:thunks_only()
@description Filter to only thunk functions
@return Query Self for method chaining
@example
local thunks = bv:query():functions():thunks_only():get()
]]
function Query:thunks_only()
    table.insert(self.steps, function(data)
        local results = {}
        for _, item in ipairs(data) do
            if item.is_thunk then
                table.insert(results, item)
            end
        end
        return results
    end)
    return self
end

--[[
@luaapi Query:exclude_thunks()
@description Filter out thunk functions
@return Query Self for method chaining
@example
local real_funcs = bv:query():functions():exclude_thunks():get()
]]
function Query:exclude_thunks()
    table.insert(self.steps, function(data)
        local results = {}
        for _, item in ipairs(data) do
            if not item.is_thunk then
                table.insert(results, item)
            end
        end
        return results
    end)
    return self
end

--[[
@luaapi Query:pure_only()
@description Filter to only pure functions (no side effects)
@return Query Self for method chaining
@example
local pure_funcs = bv:query():functions():pure_only():get()
]]
function Query:pure_only()
    table.insert(self.steps, function(data)
        local results = {}
        for _, item in ipairs(data) do
            if item.is_pure then
                table.insert(results, item)
            end
        end
        return results
    end)
    return self
end

--[[
@luaapi Query:with_stack_adjustment()
@description Filter to functions with non-zero stack adjustment
@return Query Self for method chaining
@example
local stack_funcs = bv:query():functions():with_stack_adjustment():get()
]]
function Query:with_stack_adjustment()
    table.insert(self.steps, function(data)
        local results = {}
        for _, item in ipairs(data) do
            local adj = item.stack_adjustment
            if adj and adj ~= 0 then
                table.insert(results, item)
            end
        end
        return results
    end)
    return self
end

--[[
@luaapi Query:with_variables()
@description Filter to functions that have variables
@return Query Self for method chaining
@example
local funcs_with_vars = bv:query():functions():with_variables():get()
]]
function Query:with_variables()
    table.insert(self.steps, function(data)
        local results = {}
        for _, item in ipairs(data) do
            local vars = item:variables() or {}
            if #vars > 0 then
                table.insert(results, item)
            end
        end
        return results
    end)
    return self
end

--[[
@luaapi Query:with_tags()
@description Filter to functions that have tags attached
@return Query Self for method chaining
@example
local tagged_funcs = bv:query():functions():with_tags():get()
]]
function Query:with_tags()
    table.insert(self.steps, function(data)
        local results = {}
        for _, item in ipairs(data) do
            local tags = item:get_tags() or {}
            if #tags > 0 then
                table.insert(results, item)
            end
        end
        return results
    end)
    return self
end

-- Data transformation
function Query:select(transform)
    table.insert(self.steps, function(data)
        local results = {}
        for _, item in ipairs(data) do
            table.insert(results, transform(item))
        end
        return results
    end)
    return self
end

--[[
@luaapi Query:sort_by(key_func, descending)
@description Sort results by a key function
@param key_func function Function that takes an item and returns a sortable value
@param descending boolean Optional: true for descending order, false/nil for ascending
@return Query Self for method chaining
@example
local sorted_by_size = bv:query():functions():sort_by(function(f) return f.size end, true):get()
]]
function Query:sort_by(key_func, descending)
    table.insert(self.steps, function(data)
        local sorted = {}
        for _, item in ipairs(data) do
            table.insert(sorted, item)
        end

        table.sort(sorted, function(a, b)
            local a_val = key_func(a)
            local b_val = key_func(b)
            if descending then
                return a_val > b_val
            else
                return a_val < b_val
            end
        end)

        return sorted
    end)
    return self
end

--[[
@luaapi Query:limit(count)
@description Limit results to first N items
@param count integer Maximum number of items to return
@return Query Self for method chaining
@example
local top_10 = bv:query():functions():sort_by(function(f) return f.size end, true):limit(10):get()
]]
function Query:limit(count)
    table.insert(self.steps, function(data)
        local results = {}
        for i = 1, math.min(count, #data) do
            table.insert(results, data[i])
        end
        return results
    end)
    return self
end

-- Statistical operations
function Query:count()
    table.insert(self.steps, function(data)
        return #data
    end)
    return self
end

function Query:sum_by(key_func)
    table.insert(self.steps, function(data)
        local sum = 0
        for _, item in ipairs(data) do
            sum = sum + key_func(item)
        end
        return sum
    end)
    return self
end

function Query:max_by(key_func)
    table.insert(self.steps, function(data)
        if #data == 0 then return nil end
        
        local max_item = data[1]
        local max_value = key_func(max_item)
        
        for i = 2, #data do
            local current_value = key_func(data[i])
            if current_value > max_value then
                max_value = current_value
                max_item = data[i]
            end
        end
        
        return max_item
    end)
    return self
end

--[[
@luaapi Query:execute()
@description Execute the query and return results
@return table Array of results from the query
]]
function Query:execute()
    local data = { bv = self.bv }

    for _, step in ipairs(self.steps) do
        data = step(data)
    end

    return data
end

--[[
@luaapi Query:get()
@description Convenience method to execute the query (alias for execute)
@return table Array of results from the query
@example
local results = bv:query():functions():larger_than(100):get()
]]
function Query:get()
    return self:execute()
end

-- Analysis builder for common patterns
local Analysis = {}
Analysis.__index = Analysis

function Analysis:new(bv)
    local obj = {
        bv = bv
    }
    setmetatable(obj, Analysis)
    return obj
end

-- Connectivity analysis
function Analysis:connectivity_report()
    local functions = self.bv:functions()
    local stats = {
        total_functions = #functions,
        most_connected = nil,
        most_called = nil,
        average_connectivity = 0,
        connectivity_distribution = {}
    }
    
    local total_connectivity = 0
    local max_connectivity = 0
    local max_callers = 0
    
    for _, func in ipairs(functions) do
        local calls = func:calls() or {}
        local callers = func:callers() or {}
        local connectivity = #calls + #callers
        
        total_connectivity = total_connectivity + connectivity
        
        if connectivity > max_connectivity then
            max_connectivity = connectivity
            stats.most_connected = func
        end
        
        if #callers > max_callers then
            max_callers = #callers
            stats.most_called = func
        end
        
        -- Distribution
        local bucket = "0"
        if connectivity > 0 and connectivity <= 2 then bucket = "1-2"
        elseif connectivity <= 5 then bucket = "3-5"
        elseif connectivity <= 10 then bucket = "6-10"
        else bucket = "10+" end
        
        stats.connectivity_distribution[bucket] = (stats.connectivity_distribution[bucket] or 0) + 1
    end
    
    stats.average_connectivity = total_connectivity / #functions
    
    return stats
end

-- Code size analysis
function Analysis:size_analysis()
    local functions = self.bv:functions()
    local stats = {
        total_functions = #functions,
        total_code_size = 0,
        average_size = 0,
        largest_function = nil,
        smallest_function = nil,
        size_distribution = {}
    }

    local total_size = 0
    local max_size = 0
    local min_size = math.huge

    for _, func in ipairs(functions) do
        local size = func.size
        total_size = total_size + size

        if size > max_size then
            max_size = size
            stats.largest_function = func
        end

        if size > 0 and size < min_size then
            min_size = size
            stats.smallest_function = func
        end

        -- Distribution
        local bucket = "empty"
        if size > 0 and size <= 50 then bucket = "tiny"
        elseif size <= 200 then bucket = "small"
        elseif size <= 1000 then bucket = "medium"
        elseif size <= 5000 then bucket = "large"
        else bucket = "huge" end

        stats.size_distribution[bucket] = (stats.size_distribution[bucket] or 0) + 1
    end

    stats.total_code_size = total_size
    stats.average_size = total_size / #functions

    return stats
end

--[[
@luaapi Analysis:function_classification()
@description Classify all functions by type (thunk, exported, auto, pure, etc.)
@return table Statistics about function classifications
@example
local class = bv:analyze():function_classification()
print("Exported:", class.exported)
print("Thunks:", class.thunks)
print("Auto-discovered:", class.auto_discovered)
]]
function Analysis:function_classification()
    local functions = self.bv:functions()
    local stats = {
        total_functions = #functions,
        exported = 0,
        auto_discovered = 0,
        user_defined = 0,
        thunks = 0,
        pure = 0,
        inlined = 0,
        with_tags = 0,
        has_unresolved_branches = 0,
        analysis_skipped = 0,
        exported_list = {},
        thunk_list = {},
        pure_list = {}
    }

    for _, func in ipairs(functions) do
        if func.is_exported then
            stats.exported = stats.exported + 1
            table.insert(stats.exported_list, func)
        end

        if func.auto then
            stats.auto_discovered = stats.auto_discovered + 1
        else
            stats.user_defined = stats.user_defined + 1
        end

        if func.is_thunk then
            stats.thunks = stats.thunks + 1
            table.insert(stats.thunk_list, func)
        end

        if func.is_pure then
            stats.pure = stats.pure + 1
            table.insert(stats.pure_list, func)
        end

        if func.is_inlined_during_analysis then
            stats.inlined = stats.inlined + 1
        end

        if func.has_unresolved_indirect_branches then
            stats.has_unresolved_branches = stats.has_unresolved_branches + 1
        end

        if func.analysis_skipped then
            stats.analysis_skipped = stats.analysis_skipped + 1
        end

        local tags = func:get_tags() or {}
        if #tags > 0 then
            stats.with_tags = stats.with_tags + 1
        end
    end

    return stats
end

--[[
@luaapi Analysis:tag_report()
@description Generate statistics about tags in the binary
@return table Statistics about tags and tag types
@example
local tags = bv:analyze():tag_report()
print("Total tags:", tags.total_tags)
for type_name, count in pairs(tags.by_type) do
    print(type_name, count)
end
]]
function Analysis:tag_report()
    local tags = self.bv:get_all_tags()
    local tag_types = self.bv.tag_types or {}

    local stats = {
        total_tags = #tags,
        total_tag_types = #tag_types,
        by_type = {},
        tag_types = tag_types
    }

    for _, tag in ipairs(tags) do
        local type_name = tag.type and tag.type.name or "unknown"
        stats.by_type[type_name] = (stats.by_type[type_name] or 0) + 1
    end

    return stats
end

--[[
@luaapi Analysis:stack_analysis()
@description Analyze stack usage across all functions
@return table Statistics about stack adjustments
@example
local stacks = bv:analyze():stack_analysis()
print("Max stack:", stacks.max_adjustment)
print("Average:", stacks.average_adjustment)
]]
function Analysis:stack_analysis()
    local functions = self.bv:functions()
    local stats = {
        total_functions = #functions,
        functions_with_adjustment = 0,
        max_adjustment = 0,
        min_adjustment = 0,
        total_adjustment = 0,
        average_adjustment = 0,
        largest_stack_func = nil
    }

    for _, func in ipairs(functions) do
        local adj = func.stack_adjustment
        if adj and adj ~= 0 then
            stats.functions_with_adjustment = stats.functions_with_adjustment + 1
            stats.total_adjustment = stats.total_adjustment + math.abs(adj)

            if adj > stats.max_adjustment then
                stats.max_adjustment = adj
                stats.largest_stack_func = func
            end
            if adj < stats.min_adjustment then
                stats.min_adjustment = adj
            end
        end
    end

    if stats.functions_with_adjustment > 0 then
        stats.average_adjustment = stats.total_adjustment / stats.functions_with_adjustment
    end

    return stats
end

--[[
@luaapi Analysis:variable_analysis()
@description Analyze variable usage across all functions
@return table Statistics about variables
@example
local vars = bv:analyze():variable_analysis()
print("Total variables:", vars.total_variables)
print("Functions with vars:", vars.functions_with_variables)
]]
function Analysis:variable_analysis()
    local functions = self.bv:functions()
    local stats = {
        total_functions = #functions,
        total_variables = 0,
        functions_with_variables = 0,
        max_variables = 0,
        average_variables = 0,
        most_vars_func = nil,
        by_source_type = {}
    }

    for _, func in ipairs(functions) do
        local vars = func:variables() or {}
        local var_count = #vars

        if var_count > 0 then
            stats.functions_with_variables = stats.functions_with_variables + 1
            stats.total_variables = stats.total_variables + var_count

            if var_count > stats.max_variables then
                stats.max_variables = var_count
                stats.most_vars_func = func
            end

            for _, var in ipairs(vars) do
                local src = var.source_type or "unknown"
                stats.by_source_type[src] = (stats.by_source_type[src] or 0) + 1
            end
        end
    end

    if stats.functions_with_variables > 0 then
        stats.average_variables = stats.total_variables / stats.functions_with_variables
    end

    return stats
end

--[[
@luaapi Analysis:type_analysis()
@description Analyze defined types in the binary
@return table Statistics about types
@example
local types = bv:analyze():type_analysis()
print("Structures:", types.structures)
print("Enumerations:", types.enumerations)
]]
function Analysis:type_analysis()
    local types_list = self.bv:types()
    local stats = {
        total_types = #types_list,
        structures = 0,
        enumerations = 0,
        pointers = 0,
        arrays = 0,
        functions = 0,
        other = 0,
        structure_list = {},
        enum_list = {}
    }

    for _, entry in ipairs(types_list) do
        local t = entry.type
        if t then
            if t.is_structure then
                stats.structures = stats.structures + 1
                table.insert(stats.structure_list, entry)
            elseif t.is_enumeration then
                stats.enumerations = stats.enumerations + 1
                table.insert(stats.enum_list, entry)
            elseif t.is_pointer then
                stats.pointers = stats.pointers + 1
            elseif t.is_array then
                stats.arrays = stats.arrays + 1
            elseif t.is_function then
                stats.functions = stats.functions + 1
            else
                stats.other = stats.other + 1
            end
        end
    end

    return stats
end

-- Create query builder
function fluent.query(bv)
    return Query:new(bv)
end

-- Create analysis builder
function fluent.analyze(bv)
    return Analysis:new(bv)
end

---@section BinaryView Extensions
---Extends BinaryView with fluent API methods

---Extend BinaryView with fluent methods
function fluent.extend_binaryview()
    -- Get the BinaryView metatable from the registry
    local bv_mt = debug.getregistry()["BinaryNinja.BinaryView"]
    if not bv_mt or not bv_mt.__index then
        print("Warning: Could not find BinaryView metatable for fluent API")
        return
    end

    --[[
    @luaapi BinaryView:query()
    @description Create a fluent query builder for complex analysis workflows
    @return Query Query builder instance for chaining operations
    @example
    local large_functions = bv:query()
        :functions()
        :larger_than(1000)
        :with_calls_to("malloc")
        :sort_by(function(f) return f.size end, true)
        :limit(10)
        :get()
    ]]
    bv_mt.__index.query = function(self)
        return Query:new(self)
    end

    --[[
    @luaapi BinaryView:analyze()
    @description Create an analysis helper for common analysis patterns
    @return Analysis Analysis helper instance with pre-built workflows
    @example
    local connectivity = bv:analyze():connectivity_report()
    local size_stats = bv:analyze():size_analysis()
    ]]
    bv_mt.__index.analyze = function(self)
        return Analysis:new(self)
    end
end

return fluent