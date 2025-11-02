local process = require("wf.api.v1.process")
local superfamiconv = require("wf.api.v1.process.tools.superfamiconv")
local zx0 = require("wf.api.v1.process.tools.zx0")

local tileset = superfamiconv.convert_tileset(
	"bootstub_tiles.png",
	superfamiconv.config()
		:mode("ws"):bpp(2)
		:color_zero("#ffffff")
		:no_discard():no_flip()
)

-- FIXME
-- ["section"]=".rom0_ff.bootstub_tiles"
process.emit_symbol("gfx_bootstub_tiles", tileset.tiles, {
})
