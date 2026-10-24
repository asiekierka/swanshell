local process = require("wf.api.v1.process")
local superfamiconv = require("wf.api.v1.process.tools.superfamiconv")
local zx0 = require("wf.api.v1.process.tools.zx0")

function emit_screen_pattern(name, pal_func, offset)
    local s = ""
    for y=0,17 do
        for x=0,27 do
            v = pal_func(x, y) + offset + y + x*18
            s = s .. string.char(v & 0xFF, v >> 8)
        end
        if y < 17 then
            s = s .. string.char(0, 0, 0, 0, 0, 0, 0, 0)
        end
    end
    process.emit_symbol(name, process.to_data(s), {})
end

emit_screen_pattern("ui_layout_clear_data", function(x, y) return 0 end, 0)
emit_screen_pattern("ui_layout_bars_h_data", function(x, y) if y == 0 or y == 17 then return (2 << 9) else return 0 end end, 0)
emit_screen_pattern("ui_layout_bars_v_data", function(x, y) if x == 0 or x == 27 then return (2 << 9) else return 0 end end, 0)

