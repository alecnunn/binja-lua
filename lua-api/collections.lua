--[[
@file collections.lua
@brief Idiomatic Lua collection APIs and iterators for Binary Ninja

This module provides idiomatic Lua collection APIs and iterator patterns
for working with Binary Ninja objects. It extends BinaryView and Function
objects with convenient iterator methods and fluent collection wrappers.

@example
-- Iterate over all functions
for func in bv:each_function() do
    print(func.name)
end

-- Use fluent collection API
local large_functions = bv:functions_collection()
    :filter(function(f) return f.size > 1000 end)
    :sort(function(a, b) return a.size > b.size end)
    :take(10)
]]

local collections = {}

---Helper function to create an iterator from a table
---@param table table The table to create an iterator for
---@return function Iterator function that returns next item or nil
local function make_iterator(table)
    local i = 0
    local n = #table
    return function()
        i = i + 1
        if i <= n then
            return table[i]
        end
    end
end

---@class Collection
---@brief Fluent collection wrapper for Binary Ninja objects
---
---Collection class that provides fluent API patterns for filtering, transforming,
---and analyzing collections of Binary Ninja objects like functions, basic blocks, etc.
---
---@example
---local functions = bv:functions_collection()
---    :filter(function(f) return f.size > 100 end)
---    :sort(function(a, b) return a.size > b.size end)
---    :take(5)
---    :to_table()
local Collection = {}
Collection.__index = Collection

---Create a new Collection instance
---@param items table Optional initial items for the collection
---@return Collection New collection instance
function Collection:new(items)
    local obj = {
        items = items or {}
    }
    setmetatable(obj, Collection)
    return obj
end

---Get an iterator for the collection items
---@return function Iterator function
function Collection:each()
    return make_iterator(self.items)
end

---Filter collection items based on a predicate function
---@param predicate function Function that takes an item and returns boolean
---@return Collection New filtered collection
---@example
---local large_funcs = bv:functions_collection():filter(function(f) return f.size > 1000 end)
function Collection:filter(predicate)
    local filtered = {}
    for _, item in ipairs(self.items) do
        if predicate(item) then
            table.insert(filtered, item)
        end
    end
    return Collection:new(filtered)
end

---Transform each item in the collection
---@param transform function Function that takes an item and returns transformed value
---@return Collection New collection with transformed items
---@example
---local func_names = bv:functions_collection():map(function(f) return f.name end)
function Collection:map(transform)
    local mapped = {}
    for _, item in ipairs(self.items) do
        table.insert(mapped, transform(item))
    end
    return Collection:new(mapped)
end

---Sort the collection items
---@param compare_func function Optional comparison function for custom sorting
---@return Collection New sorted collection
---@example
---local sorted_by_size = bv:functions_collection():sort(function(a, b) return a.size < b.size end)
function Collection:sort(compare_func)
    local sorted_items = {}
    for _, item in ipairs(self.items) do
        table.insert(sorted_items, item)
    end
    
    if compare_func then
        table.sort(sorted_items, compare_func)
    else
        table.sort(sorted_items)
    end
    
    return Collection:new(sorted_items)
end

---Take the first N items from the collection
---@param count number Number of items to take
---@return Collection New collection with first N items
---@example
---local first_10 = bv:functions_collection():take(10)
function Collection:take(count)
    local taken = {}
    for i = 1, math.min(count, #self.items) do
        table.insert(taken, self.items[i])
    end
    return Collection:new(taken)
end

---Skip the first N items and return the rest
---@param count number Number of items to skip
---@return Collection New collection without first N items
function Collection:skip(count)
    local skipped = {}
    for i = count + 1, #self.items do
        table.insert(skipped, self.items[i])
    end
    return Collection:new(skipped)
end

function Collection:count()
    return #self.items
end

function Collection:first(predicate)
    if not predicate then
        return self.items[1]
    end
    
    for _, item in ipairs(self.items) do
        if predicate(item) then
            return item
        end
    end
    return nil
end

function Collection:to_table()
    local result = {}
    for _, item in ipairs(self.items) do
        table.insert(result, item)
    end
    return result
end

-- Statistics and aggregation
function Collection:max_by(key_func)
    if #self.items == 0 then return nil end
    
    local max_item = self.items[1]
    local max_value = key_func(max_item)
    
    for i = 2, #self.items do
        local current_value = key_func(self.items[i])
        if current_value > max_value then
            max_value = current_value
            max_item = self.items[i]
        end
    end
    
    return max_item
end

function Collection:min_by(key_func)
    if #self.items == 0 then return nil end
    
    local min_item = self.items[1]
    local min_value = key_func(min_item)
    
    for i = 2, #self.items do
        local current_value = key_func(self.items[i])
        if current_value < min_value then
            min_value = current_value
            min_item = self.items[i]
        end
    end
    
    return min_item
end

function Collection:sum_by(key_func)
    local sum = 0
    for _, item in ipairs(self.items) do
        sum = sum + key_func(item)
    end
    return sum
end

function Collection:group_by(key_func)
    local groups = {}
    for _, item in ipairs(self.items) do
        local key = key_func(item)
        if not groups[key] then
            groups[key] = {}
        end
        table.insert(groups[key], item)
    end
    
    -- Convert to Collection objects
    local result = {}
    for key, items in pairs(groups) do
        result[key] = Collection:new(items)
    end
    return result
end

---@section BinaryView Extensions
---Extends BinaryView with iterator and collection methods

---BinaryView extensions - register on metatable
local function extend_binaryview()
    -- Get the BinaryView metatable from the registry
    local bv_mt = debug.getregistry()["BinaryNinja.BinaryView"]
    if not bv_mt or not bv_mt.__index then
        print("Warning: Could not find BinaryView metatable")
        return
    end

    --[[
    @luaapi BinaryView:each_function()
    @description Iterate over all functions in the binary
    @return function Iterator function that yields Function objects
    @example
    for func in bv:each_function() do
        print(func.name, func.size)
    end
    ]]
    bv_mt.__index.each_function = function(self)
        local functions = self:functions()
        return make_iterator(functions)
    end

    --[[
    @luaapi BinaryView:each_section()
    @description Iterate over all sections in the binary
    @return function Iterator function that yields Section objects
    @example
    for section in bv:each_section() do
        print(section.name, section.start_addr)
    end
    ]]
    bv_mt.__index.each_section = function(self)
        local sections = self:sections()
        return make_iterator(sections)
    end

    --[[
    @luaapi BinaryView:functions_collection()
    @description Get a fluent collection wrapper for all functions
    @return Collection Collection of Function objects
    @example
    local large_funcs = bv:functions_collection()
        :filter(function(f) return f.size > 1000 end)
        :sort(function(a, b) return a.size > b.size end)
    ]]
    bv_mt.__index.functions_collection = function(self)
        local functions = self:functions()
        return Collection:new(functions)
    end

    --[[
    @luaapi BinaryView:sections_collection()
    @description Get a fluent collection wrapper for all sections
    @return Collection Collection of Section objects
    ]]
    bv_mt.__index.sections_collection = function(self)
        local sections = self:sections()
        return Collection:new(sections)
    end

    --[[
    @luaapi BinaryView:each_tag()
    @description Iterate over all tags in the binary
    @return function Iterator function that yields Tag objects
    @example
    for tag in bv:each_tag() do
        print(tag.type.name, tag.data)
    end
    ]]
    bv_mt.__index.each_tag = function(self)
        local tags = self:get_all_tags()
        return make_iterator(tags)
    end

    --[[
    @luaapi BinaryView:tags_collection()
    @description Get a fluent collection wrapper for all tags
    @return Collection Collection of Tag objects
    @example
    local important_tags = bv:tags_collection()
        :filter(function(t) return t.type.name == "Important" end)
    ]]
    bv_mt.__index.tags_collection = function(self)
        local tags = self:get_all_tags()
        return Collection:new(tags)
    end

    --[[
    @luaapi BinaryView:types_collection()
    @description Get a fluent collection wrapper for all defined types
    @return Collection Collection of {name, type} entries
    @example
    local structs = bv:types_collection()
        :filter(function(t) return t.type.is_structure end)
    ]]
    bv_mt.__index.types_collection = function(self)
        local types = self:types()
        return Collection:new(types)
    end

    --[[
    @luaapi BinaryView:each_type()
    @description Iterate over all defined types in the binary
    @return function Iterator function that yields {name, type} entries
    @example
    for entry in bv:each_type() do
        print(entry.name, entry.type.type_class)
    end
    ]]
    bv_mt.__index.each_type = function(self)
        local types = self:types()
        return make_iterator(types)
    end

    --[[
    @luaapi BinaryView:each_data_var()
    @description Iterate over all data variables in the binary
    @return function Iterator function that yields DataVariable objects
    @example
    for dv in bv:each_data_var() do
        print(string.format("0x%x: %s", dv.address.value, dv.type))
    end
    ]]
    bv_mt.__index.each_data_var = function(self)
        local vars = self:data_vars()
        return make_iterator(vars)
    end

    --[[
    @luaapi BinaryView:data_vars_collection()
    @description Get a fluent collection wrapper for all data variables
    @return Collection Collection of DataVariable objects
    ]]
    bv_mt.__index.data_vars_collection = function(self)
        local vars = self:data_vars()
        return Collection:new(vars)
    end
end

---@section Function Extensions
---Extends Function with iterator and collection methods

---Function extensions - register on metatable
local function extend_function()
    -- Get the Function metatable from the registry
    local func_mt = debug.getregistry()["BinaryNinja.Function"]
    if not func_mt or not func_mt.__index then
        print("Warning: Could not find Function metatable")
        return
    end

    --[[
    @luaapi Function:each_basic_block()
    @description Iterate over all basic blocks in the function
    @return function Iterator function that yields BasicBlock objects
    @example
    for block in func:each_basic_block() do
        print(string.format("Block at 0x%x", block.start_addr))
    end
    ]]
    func_mt.__index.each_basic_block = function(self)
        local blocks = self:basic_blocks()
        return make_iterator(blocks)
    end

    --[[
    @luaapi Function:each_caller()
    @description Iterate over all functions that call this function
    @return function Iterator function that yields Function objects
    @example
    for caller in func:each_caller() do
        print("Called by:", caller.name)
    end
    ]]
    func_mt.__index.each_caller = function(self)
        local callers = self:callers()
        return make_iterator(callers)
    end

    --[[
    @luaapi Function:each_call()
    @description Iterate over all functions called by this function
    @return function Iterator function that yields Function objects
    @example
    for called_func in func:each_call() do
        print("Calls:", called_func.name)
    end
    ]]
    func_mt.__index.each_call = function(self)
        local calls = self:calls()
        return make_iterator(calls)
    end

    --[[
    @luaapi Function:basic_blocks_collection()
    @description Get a fluent collection wrapper for basic blocks
    @return Collection Collection of BasicBlock objects
    ]]
    func_mt.__index.basic_blocks_collection = function(self)
        local blocks = self:basic_blocks()
        return Collection:new(blocks)
    end

    --[[
    @luaapi Function:callers_collection()
    @description Get a fluent collection wrapper for caller functions
    @return Collection Collection of Function objects
    ]]
    func_mt.__index.callers_collection = function(self)
        local callers = self:callers()
        return Collection:new(callers)
    end

    --[[
    @luaapi Function:calls_collection()
    @description Get a fluent collection wrapper for called functions
    @return Collection Collection of Function objects
    ]]
    func_mt.__index.calls_collection = function(self)
        local calls = self:calls()
        return Collection:new(calls)
    end

    --[[
    @luaapi Function:each_variable()
    @description Iterate over all variables in the function
    @return function Iterator function that yields Variable objects
    @example
    for var in func:each_variable() do
        print(var.name, var.type)
    end
    ]]
    func_mt.__index.each_variable = function(self)
        local vars = self:variables()
        return make_iterator(vars)
    end

    --[[
    @luaapi Function:variables_collection()
    @description Get a fluent collection wrapper for all variables
    @return Collection Collection of Variable objects
    @example
    local stack_vars = func:variables_collection()
        :filter(function(v) return v.source_type == "stack" end)
    ]]
    func_mt.__index.variables_collection = function(self)
        local vars = self:variables()
        return Collection:new(vars)
    end

    --[[
    @luaapi Function:each_clobbered_reg()
    @description Iterate over registers clobbered by this function
    @return function Iterator function that yields register names
    @example
    for reg in func:each_clobbered_reg() do
        print("Clobbers:", reg)
    end
    ]]
    func_mt.__index.each_clobbered_reg = function(self)
        local regs = self:clobbered_regs()
        return make_iterator(regs)
    end

    --[[
    @luaapi Function:each_tag()
    @description Iterate over all tags attached to this function
    @return function Iterator function that yields Tag objects
    @example
    for tag in func:each_tag() do
        print(tag.type.name, tag.data)
    end
    ]]
    func_mt.__index.each_tag = function(self)
        local tags = self:get_tags()
        return make_iterator(tags)
    end

    --[[
    @luaapi Function:tags_collection()
    @description Get a fluent collection wrapper for function tags
    @return Collection Collection of Tag objects
    ]]
    func_mt.__index.tags_collection = function(self)
        local tags = self:get_tags()
        return Collection:new(tags)
    end
end

-- Initialize extensions
function collections.init()
    extend_binaryview()
    extend_function()
end

-- Export the Collection class and utilities
collections.Collection = Collection
collections.make_iterator = make_iterator

return collections