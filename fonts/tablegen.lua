local stringx = require("pl.stringx")
local tablex = require("pl.tablex")
local unicode_table = dofile("fonts/lib/unicode_table.lua")

local args = {...}
if #args < 3 then
    print("syntax: tablegen <table type> <input filename> <output filename>")
    os.exit(0)
end

local table = unicode_table.parse(args[2])
local outf <close> = io.open(args[3], "wb")

local function mapch(ch)
    local mch = table[ch]
    if mch == nil then mch = 0 end
    outf:write(string.char(mch & 0xFF, (mch >> 8) & 0xFF))
end

if args[1] == "shiftjis" then
    for i=0x81,0x9F do for j=0x40,0xFC do mapch(i * 256 + j) end end
    for i=0xE0,0xEA do for j=0x40,0xFC do mapch(i * 256 + j) end end
else
    for i=0,65535 do mapch(i) end
end