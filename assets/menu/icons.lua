local process = require("wf.api.v1.process")
local nnpack = require("wf.api.v1.process.tools.nnpack")
local superfamiconv = require("wf.api.v1.process.tools.superfamiconv")

local tileset = superfamiconv.convert_tileset(
	"icons.png",
	superfamiconv.config()
		:mode("ws"):bpp(2)
		:color_zero("#ffffff")
		:no_discard():no_flip()
)

process.emit_symbol("gfx_icons", tileset)
