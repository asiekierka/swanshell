local MAX_WIDTH = 15
local MAX_HEIGHT = 15
local ROM_OFFSET_SHIFT = 0

local bdf = dofile("fonts/lib/bdf.lua")
local stringx = require("pl.stringx")
local tablex = require("pl.tablex")
local unicode_table = dofile("fonts/lib/unicode_table.lua")

local function table_to_string(n)
    return string.char(table.unpack(n))
end

local function add_char_gap(font, amount)
    for id, char in pairs(font.chars) do
        char.gap = (char.gap or 0) + amount
    end                
end

local function build_font_entry_tables(height, tiny_font, fonts, x_offset_tbl, y_offset, is_allowed_char)
    local chars = {}
    local i = 0
    local max_glyph_id = 0

    for font_idx, font in pairs(fonts) do
        local x_offset = x_offset_tbl[font_idx]
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
                local char_bitwidth = ((char.width + 7) & 0xFFF8)

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

                char.width = char.width + (char.gap or 0)

                -- calculate xofs, yofs, width, height
                local res = {}
                res.width = char.width
                res.height = #bitmap
                res.bitmap = {}
                -- pack ROM data
                local x = 0
                local i = 0
                for iy=1,#bitmap do
                    local b = bitmap[iy] >> (char_bitwidth - char.width)
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
    local rom_offsets = {}
    local glyph_count_per = {}

    for i=0,max_glyph_id+1,GLYPH_TABLE_PER do
        rom_datas[i // GLYPH_TABLE_PER] = {0, 0}
        glyph_count_per[i // GLYPH_TABLE_PER] = 0
    end
    for i,char in pairs(chars) do
        glyph_count_per[i // GLYPH_TABLE_PER] = glyph_count_per[i // GLYPH_TABLE_PER] + 1
    end

    local function use_small_glyph(id)
        return (id == 0 and not tiny_font) or glyph_count_per[id] >= GLYPH_ENTRY_STATIC_LIMIT
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

        rom_offsets[id] = header_pos - 1

        local v1 = ((rom_offset - header_pos + 1) >> ROM_OFFSET_SHIFT)
        rom_data[header_pos] = v1 & 0xFF
        rom_data[header_pos + 1] = (v1 >> 8) & 0xFF
        rom_data[header_pos + 2] = char.x | (char.y << 4)
        rom_data[header_pos + 3] = char.width | (char.height << 4)
    end

    return rom_datas, rom_offsets
end

local function write_font(filename, font_height, tables, offsets)
    local max_codepoint = math.max(table.unpack(tablex.keys(tables)))
    local empty_pointer = string.char(0, 0, 0)
    local file <close> = io.open(filename, "wb")
    file:write(string.pack("<HHHHI3B",
        0x6653, 0x0100, 12, max_codepoint, 0, font_height
    ))
    local pointers_fpos = file:seek()
    file:write(empty_pointer:rep(max_codepoint+1))
    -- TODO: Optimize by populating smaller, empty banks
    pointer_locs = {}
    qmark_pos = 0
    for i=0,max_codepoint do
        local data = tables[i]
        local fpos = file:seek()
        local fpos_left = 0x10000 - (fpos & 0xFFFF)
        if fpos_left < #data then
            file:write(string.char(0):rep(fpos_left))
            fpos = file:seek()
        end
        file:write(string.char(table.unpack(data)))
        pointer_locs[i] = fpos
        if i == 0 then
            qmark_pos = fpos + offsets[63]
        end
    end
    file:seek("set", 8)
    file:write(string.pack("<I3", qmark_pos))
    file:seek("set", pointers_fpos)
    for i=0,max_codepoint do
        file:write(string.pack("<I3", pointer_locs[i]))
    end
end

local args = {...}
if #args < 3 then
    print("syntax: builder <font type> <font language code> <output filename>")
    os.exit(0)
end

local function filter_default(ch, font)
    -- control codes
    if (ch < 0x20) or (ch >= 0x80 and ch < 0xA0) then return false end 
    -- Braille
    if ch >= 0x2800 and ch < 0x2900 then return false end
    -- UTF-16 surrogates, PUA
    if ch >= 0xD800 and ch < 0xF900 then return false end
    return true
end

local function filter_tiny(ch, font)
    return ch >= 0x20 and ch < 0x80
end

if args[1] == "default8" then
    local swanshell7 = bdf.parse("fonts/local/swanshell_7px.bdf")
    local misaki = bdf.parse("fonts/misaki/misaki_gothic_2nd.bdf")
    local boutiquebitmap7 = bdf.parse("fonts/boutique/BoutiqueBitmap7x7_1.7.bdf")
    -- local dalmoori = bdf.parse("fonts/dalmoori/dalmoori.bdf")

    add_char_gap(swanshell7, 1)
    add_char_gap(misaki, 1)
    add_char_gap(boutiquebitmap7, 1)

    local font_order = {swanshell7, misaki, boutiquebitmap7}
    local font_offsets = {0, 0, 0}
    if stringx.startswith(args[2], "zh_") then
        font_order = {swanshell7, boutiquebitmap7, misaki}
    end

    write_font(args[3], 8, build_font_entry_tables(8, false, font_order, font_offsets, 0, filter_default))
end

if args[1] == "default16" then
    local arkpixel12
    if args[2] == "zh_hans" then
        arkpixel12 = bdf.parse("fonts/build/ark-pixel-12px-proportional-zh_cn.bdf")
    elseif args[2] == "zh_hant" then
        arkpixel12 = bdf.parse("fonts/build/ark-pixel-12px-proportional-zh_tw.bdf")
    else
        arkpixel12 = bdf.parse("fonts/build/ark-pixel-12px-proportional-ja.bdf")
    end
    local ksx1001table = unicode_table.parse("fonts/tables/KSX1001.TXT")
    local baekmukdotum12 = bdf.parse("fonts/baekmuk/dotum12.bdf", ksx1001table)

    add_char_gap(arkpixel12, 1)
    add_char_gap(baekmukdotum12, 1)

    write_font(args[3], 16, build_font_entry_tables(16, false, {arkpixel12, baekmukdotum12}, {-1, 0}, nil, filter_default))
end

if args[1] == "tiny8" then
    local font = bdf.parse("fonts/local/swanshell_7px.bdf")
    add_char_gap(font, 1)
    write_font(args[3], 8, build_font_entry_tables(8, true, {font}, {0}, 0, filter_tiny))
end


if args[1] == "tiny16" then
    local font = bdf.parse("fonts/build/ark-pixel-12px-proportional-ja.bdf")
    add_char_gap(font, 1)
    write_font(args[3], 16, build_font_entry_tables(16, true, {font}, {-1}, -3, filter_tiny))
end
