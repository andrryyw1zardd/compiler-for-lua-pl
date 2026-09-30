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

local intLit       = 42
local floatLit     = 3.14
local hexLit       = 0xFF
local hexFloat     = 0x1p4        -- hex float (16.0)
local sciLit       = 1.5e3        -- 1500.0
local sci2         = 2e3
local sci3         = 1.e3
local sci4         = .5e3
local sci5         = 1E3
local sci6         = 1e+3
local sci7         = 1e-3
local sci8         = 1.5E-10
local sci9         = 0.5e2
local sci10        = 314.159e-2
local negLit       = -7
local Vector = {}

print("\n-- Numeric literals --")
print(intLit, floatLit, hexLit, hexFloat, sciLit, negLit)
print("math.type(intLit) =", math.type(intLit))
print("math.type(floatLit) =", math.type(floatLit))

local singleQuoted = 'single quoted'
local doubleQuoted = "double\tquoted\nwith escapes"
local longString = [[
This is a long string.
It can span multiple lines
without needing escape characters.
]]
local longStringLevel = [==[ long string with ]] inside ]==]

print("\n-- Strings --")
print(singleQuoted)
print(doubleQuoted)
print(longString)
print(longStringLevel)

function Vector:new(x, y)
    return x + y 
end

local res = Vector:new(1, 2)

--------------------------------------------------------------------
-- 4. repeat / until
--------------------------------------------------------------------
print("\n-- Repeat/Until --")
local i = 0
repeat
    i = i + 1
    print("repeat i =", i)
until i >= 3

--------------------------------------------------------------------
-- 7. Varargs (...)
--------------------------------------------------------------------
local function sum(...)
    local total = 0
    for _, v in ipairs({...}) do
        total = total + v
    end
    return total
end

local function countArgs(...)
    return select("#", ...)
end

print("\n-- Varargs --")
print("sum(1,2,3,4) =", sum(1, 2, 3, 4))
print("countArgs(1,2,3) =", countArgs(1, 2, 3))
print("select(2, 'a','b','c') =", select(2, "a", "b", "c"))

--------------------------------------------------------------------
-- 8. Closures / Upvalues
--------------------------------------------------------------------
local function makeCounter()
    local count = 0
    return function()
        count = count + 1
        return count
    end
end

print("\n-- Closures --")
local counter1 = makeCounter()
local counter2 = makeCounter()
print(counter1(), counter1(), counter1())
print(counter2())

--------------------------------------------------------------------
-- 9. Recursive local functions
--------------------------------------------------------------------
local function factorial(n)
    if n <= 1 then return 1 end
    return n * factorial(n - 1)
end

print("\n-- Recursion --")
print("factorial(6) =", factorial(6))
