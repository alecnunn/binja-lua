-- init.lua
-- Auto-initialization for Binary Ninja Lua API extensions

local collections = require('collections')
local fluent = require('fluent')
local utils = require('utils')

-- Auto-initialize extensions when this module is loaded
collections.init()
fluent.extend_binaryview()
utils.init()

-- Export all modules
return {
    collections = collections,
    fluent = fluent,
    utils = utils
}