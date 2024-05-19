local process = require("wf.api.v1.process")
local nnpack = require("wf.api.v1.process.tools.nnpack")
local superfamiconv = require("wf.api.v1.process.tools.superfamiconv")

local function gen_palette_mono(values)
	local pal = values[1] | (values[2] << 4) | (values[3] << 8) | (values[4] << 12)
	return process.to_data(string.char(pal & 0xFF, pal >> 8))
end

local function gen_palette_color(values)
	local pal = ""
	for i = 1,16 do
		local p = values[i] or 0xFFF
		pal = pal .. string.char(p & 0xFF, p >> 8)
	end
	return process.to_data(pal)
end

-- icon palette should use colors 2 and 3 for bg and text
local palette_mono = gen_palette_mono({2, 5, 0, 7})
local palette_color = gen_palette_color({
	0xF0F, 0x616, 0xFFF, 0x000,
	0xC43, 0xF91, 0xFF3, 0x9F4,
	0x1D6, 0x169, 0x219, 0x22F,
	0x27F, 0x5DF, 0xAAA, 0x555
})

process.emit_symbol("gfx_icons_palmono", palette_mono)
process.emit_symbol("gfx_icons_8mono", superfamiconv.tiles(
	"icons/8mono.png", palette_mono,
	superfamiconv.config()
		:mode("ws"):bpp(2)
		:color_zero("#ffffff")
		:no_discard():no_flip()
))
process.emit_symbol("gfx_icons_16mono", superfamiconv.tiles(
	"icons/16mono.png", palette_mono,
	superfamiconv.config()
		:mode("ws"):bpp(2)
		:color_zero("#ffffff")
		:no_discard():no_flip()
))

process.emit_symbol("gfx_icons_palcolor", palette_color)
process.emit_symbol("gfx_icons_8color", superfamiconv.tiles(
	"icons/8color.png", palette_color,
	superfamiconv.config()
		:mode("wsc"):bpp(4)
		:color_zero("#ffffff")
		:no_discard():no_flip()
))
process.emit_symbol("gfx_icons_16color", superfamiconv.tiles(
	"icons/16color.png", palette_color,
	superfamiconv.config()
		:mode("wsc"):bpp(4)
		:color_zero("#ffffff")
		:no_discard():no_flip()
))

