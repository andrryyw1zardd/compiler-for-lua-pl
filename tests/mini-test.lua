local Vector = {}

function Vector.new(x, y)
    return x + y 
end

local res = Vector.new(1, 2)

--[[
local a = 10
local b, c = 20, "Hello"

print(a)

function foo()
  print(b, c)
end

if (b > a) then 
  print(b)
else 
  print(a)
end
]]
