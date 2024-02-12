local MAX_WIDTH = 15
local MAX_HEIGHT = 15
-- local Y_OFFSET = -2
local Y_OFFSET = 0
local GLYPH_TABLE_SHIFT = 16
local GLYPH_TABLE_PER = (1 << GLYPH_TABLE_SHIFT)

local bdf = dofile("lib/bdf.lua")
local nnpack = require("wf.api.v1.process.tools.nnpack")
local process = require("wf.api.v1.process")
local tablex = require("pl.tablex")

local llimit = bdf.parse("fonts/misaki_gothic_2nd.bdf")
local fonts = {llimit}

local function is_allowed_char(ch, font)
    -- BMP only
    if ch >= 0x10000 then return false end
    -- control codes
    if (ch < 0x20) or (ch >= 0x80 and ch < 0xA0) then return false end 
    -- Braille
    if ch >= 0x2800 and ch < 0x2900 then return false end
    -- UTF-16 surrogates, PUA
    if ch >= 0xD800 and ch < 0xF900 then return false end
    return true
end

local chars = {}
local i = 0
local max_glyph_id = 0

for _, font in pairs(fonts) do
    for id, char in pairs(font.chars) do
        if chars[id] == nil and is_allowed_char(id, font) then
            if id == 0x20 then char.width = 2 end
            local bitmap = char.bitmap
            if char.height > MAX_HEIGHT then
                -- remove empty rows
                bitmap = {}
                for i=1,#char.bitmap do
                    if char.bitmap[i] ~= 0 then
                        table.insert(bitmap, char.bitmap[i])
                    end
                end

                if #bitmap > MAX_HEIGHT then
                    print(char.name .. " too tall, skipping")
                    goto nextchar
                end
            end
            if char.width > MAX_WIDTH then
                print(char.name .. " too wide, skipping")
                goto nextchar
            end

            -- calculate xofs, yofs, width, height
            local res = {}
            res.width = char.width
            res.height = #bitmap
            res.bitmap = {}
            -- pack ROM data
            local x = 0
            local i = 0
            for iy=1,char.height do
                local b = char.bitmap[iy] >> (((char.width + 7) & 0xFFF8) - char.width)
                local m = 1
                for ix=1,char.width do
                    if (b & m) ~= 0 then
                        x = x | (1 << i)
                    end
                    i = i + 1
                    m = m << 1
                    if i == 8 then
                        table.insert(res.bitmap, x)
                        x = 0
                        i = 0
                    end
                end
            end
            if i > 0 then
                table.insert(res.bitmap, x)
            end
            -- pad to 2 bytes
            if (#res.bitmap & 1) ~= 0 then
                table.insert(res.bitmap, 0)
            end
            res.x = char.x
            res.y = font.ascent - char.y - char.height + Y_OFFSET
            if res.x < 0 then res.x = 0 end
            if res.y < 0 then res.y = 0 end
            if (res.y + res.height) > MAX_HEIGHT then
                res.y = MAX_HEIGHT - res.height
            end
            if (res.x > 0 or res.width > 0) then
                chars[id] = res
                if id > max_glyph_id then
                    max_glyph_id = id
                end
            end
        end
        ::nextchar::
    end
end

-- build ROM data
local rom_header = {}
for i=0,max_glyph_id+1,GLYPH_TABLE_PER do
    rom_header[i // GLYPH_TABLE_PER] = {}
end
local rom_data = {}
local glyph_id_mask = GLYPH_TABLE_PER - 1
for id, char in tablex.sort(chars) do
    -- add rom data
    local rom_offset = nil
    --[[ for i=1,#rom_data-#char.bitmap+1 do
        rom_offset = i
        for j=1,#char.bitmap do
            if char.bitmap[j] ~= rom_data[i+j-1] then
                rom_offset = nil
                break
            end
        end
    end ]]
    if rom_offset == nil then
        rom_offset = #rom_data
        for i=1,#char.bitmap do
            table.insert(rom_data,char.bitmap[i])
        end
    end

    -- add rom header
    local header = rom_header[id >> GLYPH_TABLE_SHIFT]
    local v1 = ((rom_offset >> 1) << GLYPH_TABLE_SHIFT) | (id & glyph_id_mask)
    table.insert(header, v1 & 0xFF)
    table.insert(header, (v1 >> 8) & 0xFF)
    table.insert(header, (v1 >> 16) & 0xFF)
    table.insert(header, (v1 >> 24) & 0xFF)
    table.insert(header, char.x | (char.y << 4))
    table.insert(header, char.width | (char.height << 4))
end
print(((65536 >> GLYPH_TABLE_SHIFT) * 4))
local sum = 0
for i=0,max_glyph_id+1,GLYPH_TABLE_PER do
    sum = sum + #rom_header[i // GLYPH_TABLE_PER]
end
print(sum)
print(#rom_data)

local function table_to_string(n)
    return string.char(table.unpack(n))
end

process.emit_symbol("font_table", table_to_string(rom_header[0]))
process.emit_symbol("font_bitmap", table_to_string(rom_data))
