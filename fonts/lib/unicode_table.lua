-- SPDX-License-Identifier: MIT
-- SPDX-FileContributor: Adrian "asie" Siekierka, 2026

local stringx = require("pl.stringx")

--- Unicode table loader.
-- @module wf.internal.font.unicode_table
-- @alias M

local M = {}

M.parse = function(filename, from, to)
    local file <close> = io.open(filename)
    local trans_table = {}
    if from == true then
        from = 2
        to = 1
    elseif from == nil or type(from) == "boolean" then
        from = 1
        to = 2
    end
    
    if reverse == nil then reverse = false end

    for line in file:lines() do
        if not stringx.startswith(line, "#") then
            local parts = stringx.split(line)
            trans_table[tonumber(parts[from])] = tonumber(parts[to])
        end
    end

    return trans_table
end

return M
