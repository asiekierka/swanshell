-- Copyright (C) 2026 Adrian "asie" Siekierka
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
-- copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in
-- all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
-- THE SOFTWARE.

local libplum = require("libplum")
local args = {...}

local src_img = libplum.loadfile(args[1], 0)
local dst_img = libplum.new(src_img.width, src_img.height, 1, 0)

local tile_w = tonumber(args[3])
local tile_h = tonumber(args[4])

for iy=0,src_img.height,tile_h do
  for ix=0,src_img.width,tile_w do
    for ty=0,tile_h-1 do
      for tx=0,tile_w-1 do
        local src_x = ix+tx
        local src_y = iy+ty
        local dst_x = ix+tile_h-1-ty
        local dst_y = iy+tx
        dst_img:set(dst_x, dst_y, src_img:get(src_x, src_y))
      end
    end
  end
end

dst_img.type = libplum.IMAGE_PNG
dst_img:storefile(args[2])
