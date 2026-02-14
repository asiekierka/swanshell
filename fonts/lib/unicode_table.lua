-- SPDX-License-Identifier: MIT
-- SPDX-FileContributor: Adrian "asie" Siekierka, 2026

local stringx = require("pl.stringx")

--- Unicode table loader.
-- @module wf.internal.font.unicode_table
-- @alias M

local M = {}

M.parse = function(filename, reverse)
    local file <close> = io.open(filename)
    local trans_table = {}
    if reverse == nil then reverse = false end

    for line in file:lines() do
        if not stringx.startswith(line, "#") then
            local parts = stringx.split(line)
            if reverse == true then
                trans_table[tonumber(parts[2])] = tonumber(parts[1])
            else
                trans_table[tonumber(parts[1])] = tonumber(parts[2])
            end
        end
    end

    return trans_table
end

return M
