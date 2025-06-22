local MAX_WIDTH = 15
local MAX_HEIGHT = 15
local ROM_OFFSET_SHIFT = 0

local bdf = dofile("lib/bdf.lua")
local process = require("wf.api.v1.process")
local tablex = require("pl.tablex")

local function table_to_string(n)
    return string.char(table.unpack(n))
end

local function build_font(name, height, fonts, x_offset, y_offset, is_allowed_char)
    local chars = {}
    local i = 0
    local max_glyph_id = 0

    for _, font in pairs(fonts) do
        if y_offset == nil then
            local a_char = font.chars[91] -- [
            local a_y = font.ascent - a_char.y - a_char.height
            y_offset = -a_y
            for i=1,#a_char.bitmap do
                if a_char.bitmap[i] ~= 0 then
                    break
                end
                y_offset = y_offset - 1
            end
            print("calculated automatic offset:", y_offset)
        end

        for id, char in pairs(font.chars) do
            if chars[id] == nil and is_allowed_char(id, font) then
                if id == 0x20 then
                    char.x = 0
                    char.width = 2
                    char.y = 0
                    char.height = 0
                end
                local bitmap_first_nonempty = #char.bitmap + 1
                local bitmap_last_nonempty = #char.bitmap
                local bitmap = char.bitmap
                local char_start_y = font.ascent - char.y - char.height + y_offset

                for i=1,#char.bitmap do
                    if char.bitmap[i] ~= 0 then
                        bitmap_first_nonempty = i
                        break
                    end
                end
                if bitmap_first_nonempty > bitmap_last_nonempty then
                    char.y = 0
                    char.height = 0
                else
                    for i=#char.bitmap,1,-1 do
                        if char.bitmap[i] ~= 0 then
                            bitmap_last_nonempty = i
                            break
                        end
                    end
                    -- trim empty rows
                    if bitmap_first_nonempty > 1 or bitmap_last_nonempty < #bitmap then
                        bitmap = {}
                        for i=bitmap_first_nonempty,bitmap_last_nonempty do
                            table.insert(bitmap, char.bitmap[i])
                        end
                        char_start_y = char_start_y + bitmap_first_nonempty - 1
                    end
                    char.width = char.width + x_offset
                end
                if #bitmap > MAX_HEIGHT then
                    -- remove all empty rows
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
                for iy=1,#bitmap do
                    local b = bitmap[iy] >> (((char.width + 7) & 0xFFF8) - char.width)
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
                while (#res.bitmap & ((1 << ROM_OFFSET_SHIFT) - 1)) ~= 0 do
                    table.insert(res.bitmap, 0)
                end
                res.x = char.x
                res.y = char_start_y
                --- if id == 97 then
                ---    print(res.x, res.y, res.width, res.height)
                ---    print(font.ascent, char.y, char.height)
                ---    print(table.unpack(bitmap))
                --- end
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
    local GLYPH_TABLE_SHIFT = 8
    local GLYPH_TABLE_PER = (1 << GLYPH_TABLE_SHIFT)
    local GLYPH_ENTRY_SIZE = 5
    local SMALL_GLYPH_ENTRY_SIZE = 4
    local GLYPH_ENTRY_STATIC_LIMIT = 0xCC

    local rom_datas = {}
    local glyph_count_per = {}

    for i=0,max_glyph_id+1,GLYPH_TABLE_PER do
        rom_datas[i // GLYPH_TABLE_PER] = {0, 0}
        glyph_count_per[i // GLYPH_TABLE_PER] = 0
    end
    for i,char in pairs(chars) do
        glyph_count_per[i // GLYPH_TABLE_PER] = glyph_count_per[i // GLYPH_TABLE_PER] + 1
    end

    local function use_small_glyph(id)
        return id == 0 or glyph_count_per[id] >= GLYPH_ENTRY_STATIC_LIMIT
    end

    local glyph_id_mask = GLYPH_TABLE_PER - 1
    -- preallocate room
    for id, char in pairs(chars) do
        if not use_small_glyph(id >> GLYPH_TABLE_SHIFT) then
            local rom_data = rom_datas[id >> GLYPH_TABLE_SHIFT]
            for i=1,GLYPH_ENTRY_SIZE do
                table.insert(rom_data,0)
            end
        end
    end
    for i=0,max_glyph_id+1,GLYPH_TABLE_PER do
        if use_small_glyph(i >> GLYPH_TABLE_SHIFT) then
            local rom_data = rom_datas[i >> GLYPH_TABLE_SHIFT]
            rom_data[1] = 0xFF
            rom_data[2] = 0xFF
            for i=1,GLYPH_TABLE_PER*SMALL_GLYPH_ENTRY_SIZE do
                table.insert(rom_data,0)
            end
        end
    end

    for id, char in tablex.sort(chars) do
        local rom_data = rom_datas[id >> GLYPH_TABLE_SHIFT]
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

        local font_count = rom_data[1] + (rom_data[2] << 8)
        local header_pos
        if not use_small_glyph(id >> GLYPH_TABLE_SHIFT) then
            header_pos = 4 + (font_count * GLYPH_ENTRY_SIZE)
            rom_data[header_pos - 1] = id & glyph_id_mask

            font_count = font_count + 1
            rom_data[1] = font_count & 0xFF
            rom_data[2] = font_count >> 8
        else
            header_pos = 3 + ((id & glyph_id_mask) * SMALL_GLYPH_ENTRY_SIZE)
        end

        local v1 = ((rom_offset - header_pos + 1) >> ROM_OFFSET_SHIFT)
        rom_data[header_pos] = v1 & 0xFF
        rom_data[header_pos + 1] = (v1 >> 8) & 0xFF
        rom_data[header_pos + 2] = char.x | (char.y << 4)
        rom_data[header_pos + 3] = char.width | (char.height << 4)
    end

    for k,v in pairs(rom_datas) do
        process.emit_symbol(name .. "_" .. k, table_to_string(v), {align=16})
    end
end

local swanshell7 = bdf.parse("fonts/swanshell_7px.bdf")
local misaki = bdf.parse("fonts/misaki_gothic_2nd.bdf")
local arkpixel12 = bdf.parse("fonts/ark-pixel-12px-proportional-ja.bdf")
build_font("font8_bitmap", 8, {swanshell7, misaki}, 0, 0, function(ch, font)
    -- BMP only
    -- if ch >= 0x10000 then return false end
    -- control codes
    if (ch < 0x20) or (ch >= 0x80 and ch < 0xA0) then return false end 
    -- Braille
    if ch >= 0x2800 and ch < 0x2900 then return false end
    -- UTF-16 surrogates, PUA
    if ch >= 0xD800 and ch < 0xF900 then return false end
    return true
end)
build_font("font16_bitmap", 16, {arkpixel12}, -1, nil, function(ch, font)
    -- BMP only
    -- if ch >= 0x10000 then return false end
    -- control codes
    if (ch < 0x20) or (ch >= 0x80 and ch < 0xA0) then return false end 
    -- Braille
    if ch >= 0x2800 and ch < 0x2900 then return false end
    -- UTF-16 surrogates, PUA
    if ch >= 0xD800 and ch < 0xF900 then return false end
    return true
end)
