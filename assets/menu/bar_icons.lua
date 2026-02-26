local process = require("wf.api.v1.process")
local superfamiconv = require("wf.api.v1.process.tools.superfamiconv")

local function gen_palette_mono(values)
    local pal = values[1]| (values[2] << 4) | (values[3] << 8) | (values[4] << 12)
    return process.to_data(string.char(pal & 0xFF, pal >> 8))
end

-- icon palette should use colors 2 and 3 for bg and text
local palette_mono = gen_palette_mono({ 2, 5, 7, 0 })

local function skip_second_layer(data)
    data = process.to_data(data).data
    local new_data = ""
    for i = 1, #data, 2 do
        new_data = new_data .. data:sub(i, i)
    end
    return new_data
end

process.emit_symbol("gfx_bar_icons", skip_second_layer(superfamiconv.tiles(
    "bar_icons.png", palette_mono,
    superfamiconv.config()
    :mode("ws"):bpp(2)
    :color_zero("#ffffff")
    :no_discard():no_flip()
)))
