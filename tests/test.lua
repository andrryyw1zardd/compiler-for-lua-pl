-- Extended Lua Syntax Examples
-- Covers constructs NOT present in the original test.lua

--------------------------------------------------------------------
-- 1. Comments
--------------------------------------------------------------------
-- single line comment
--[[
  multi-line (long) comment
  spans several lines
]]
--[==[
--[===[ ]===]
  long comment with a custom level (in case ]] appears inside)
]==]

--------------------------------------------------------------------
-- 2. Numeric literals
--------------------------------------------------------------------
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

print("\n-- Numeric literals --")
print(intLit, floatLit, hexLit, hexFloat, sciLit, negLit)
print("math.type(intLit) =", math.type(intLit))
print("math.type(floatLit) =", math.type(floatLit))

--------------------------------------------------------------------
-- 3. String literals & escapes
--------------------------------------------------------------------
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

--------------------------------------------------------------------
-- 10. Anonymous functions & higher-order functions
--------------------------------------------------------------------
local function map(t, f)
    local result = {}
    for i, v in ipairs(t) do
        result[i] = f(v)
    end
    return result
end

print("\n-- Higher-order functions --")
local squares = map({1, 2, 3, 4}, function(x) return x * x end)
for _, v in ipairs(squares) do io.write(v .. " ") end
print()

--------------------------------------------------------------------
-- 11. Logical operators / ternary-style idiom
--------------------------------------------------------------------
local a, b = 5, 10
local max = (a > b) and a or b
print("\n-- Ternary idiom --")
print("max =", max)
print("and/or:", true and "yes" or "no", false and "yes" or "no")

--------------------------------------------------------------------
-- 12. Bitwise operators (Lua 5.3+)
--------------------------------------------------------------------
print("\n-- Bitwise operators --")
print("5 & 3 =", 5 & 3)
print("5 | 2 =", 5 | 2)
print("5 ~ 1 =", 5 ~ 1)   -- xor
print("~5 =", ~5)          -- bitwise not
print("1 << 4 =", 1 << 4)
print("256 >> 4 =", 256 >> 4)

--------------------------------------------------------------------
-- 13. Integer division and modulo
--------------------------------------------------------------------
print("\n-- Integer division / modulo --")
print("7 // 2 =", 7 // 2)
print("7.0 // 2 =", 7.0 // 2)
print("7 % 2 =", 7 % 2)
print("-7 % 3 =", -7 % 3)

--------------------------------------------------------------------
-- 14. String library & pattern matching
--------------------------------------------------------------------
print("\n-- String library --")
local s = "Hello, Lua World!"
print(string.upper(s))
print(s:lower())                    -- method-call syntax
print(string.sub(s, 1, 5))
print(string.len(s), #s)
print(string.rep("ab", 3))
print(string.format("Pi is %.2f, name=%s, n=%d", 3.14159, "Lua", 42))
print(string.find(s, "Lua"))
print(string.match(s, "%a+, (%a+)"))
print((string.gsub(s, "o", "0")))

print("\n-- gmatch iterator --")
for word in string.gmatch("the quick brown fox", "%a+") do
    io.write(word .. "|")
end
print()

--------------------------------------------------------------------
-- 15. Table library
--------------------------------------------------------------------
print("\n-- Table library --")
local t = {5, 3, 8, 1, 9}
table.insert(t, 100)
table.insert(t, 1, -1)
table.remove(t, 2)
table.sort(t)
print("sorted:", table.concat(t, ", "))

table.sort(t, function(x, y) return x > y end)
print("sorted desc:", table.concat(t, ", "))

local packed = table.pack(1, 2, 3)
print("packed.n =", packed.n)
print("unpack:", table.unpack({10, 20, 30}))

--------------------------------------------------------------------
-- 16. Mixed table constructors (array + hash + computed keys)
--------------------------------------------------------------------
local mixed = {
    "first",
    "second",
    name = "mixed table",
    [10] = "explicit index 10",
    ["computed " .. "key"] = true,
    nested = { a = 1, b = { c = 2 } },
}
print("\n-- Mixed table constructor --")
print(mixed[1], mixed[2], mixed.name, mixed[10])
print(mixed["computed key"], mixed.nested.b.c)

--------------------------------------------------------------------
-- 17. Multiple assignment & swapping
--------------------------------------------------------------------
print("\n-- Multiple assignment --")
local x, y, z = 1, 2, 3
x, y = y, x
print(x, y, z)

--------------------------------------------------------------------
-- 18. Error handling: pcall, xpcall, error, assert
--------------------------------------------------------------------
print("\n-- Error handling --")
local ok, err = pcall(function() error("something went wrong") end)
print("pcall result:", ok, err)

local ok2, result = pcall(function() return 10 / 0 end)
print("division:", ok2, result)

local function mayFail(x)
    assert(type(x) == "number", "x must be a number")
    return x * 2
end
local ok3, err3 = pcall(mayFail, "not a number")
print("assert failure:", ok3, err3)

local function handler(e)
    return "handled: " .. tostring(e)
end
local ok4, res4 = xpcall(function() error("boom") end, handler)
print("xpcall:", ok4, res4)

--------------------------------------------------------------------
-- 19. Metatables: full metamethod set
--------------------------------------------------------------------
print("\n-- Metamethods --")
local Vector = {}
Vector.__index = Vector

function Vector.new(x, y)
    return setmetatable({x = x, y = y}, Vector)
end

Vector.__add = function(a, b) return Vector.new(a.x + b.x, a.y + b.y) end
Vector.__sub = function(a, b) return Vector.new(a.x - b.x, a.y - b.y) end
Vector.__eq  = function(a, b) return a.x == b.x and a.y == b.y end
Vector.__lt  = function(a, b) return (a.x^2 + a.y^2) < (b.x^2 + b.y^2) end
Vector.__tostring = function(v) return "(" .. v.x .. ", " .. v.y .. ")" end
Vector.__concat = function(a, b) return tostring(a) .. tostring(b) end
Vector.__len = function(v) return math.floor(math.sqrt(v.x^2 + v.y^2)) end
Vector.__call = function(v, scalar) return Vector.new(v.x * scalar, v.y * scalar) end
Vector.__unm = function(v) return Vector.new(-v.x, -v.y) end

local v1 = Vector.new(1, 2)
local v2 = Vector.new(3, 4)
print("v1 + v2 =", tostring(v1 + v2))
print("v1 - v2 =", tostring(v1 - v2))
print("v1 == Vector.new(1,2):", v1 == Vector.new(1, 2))
print("v1 < v2:", v1 < v2)
print("concat:", v1 .. v2)
print("#v2 (len) =", #v2)
print("v1(10) =", tostring(v1(10)))
print("-v1 =", tostring(-v1))

-- __newindex and __index as functions (proxy tables)
local proxy = {}
local hidden = {}
setmetatable(proxy, {
    __index = function(t, k)
        print("Accessed key:", k)
        return hidden[k]
    end,
    __newindex = function(t, k, v)
        print("Set key:", k, "to", v)
        hidden[k] = v
    end
})
proxy.foo = "bar"
print("proxy.foo =", proxy.foo)

--------------------------------------------------------------------
-- 20. Inheritance chains (multiple levels)
--------------------------------------------------------------------
print("\n-- Inheritance chain --")
local Base = {}
Base.__index = Base
function Base.new() return setmetatable({}, Base) end
function Base:hello() print("Hello from Base") end

local Mid = setmetatable({}, {__index = Base})
Mid.__index = Mid
function Mid.new() return setmetatable({}, Mid) end
function Mid:greet() print("Greet from Mid") end

local Derived = setmetatable({}, {__index = Mid})
Derived.__index = Derived
function Derived.new() return setmetatable({}, Derived) end

local d = Derived.new()
d:hello()  -- inherited from Base
d:greet()  -- inherited from Mid

--------------------------------------------------------------------
-- 21. Coroutines
--------------------------------------------------------------------
print("\n-- Coroutines --")
local co = coroutine.create(function(a, b)
    print("coroutine start", a, b)
    local x = coroutine.yield(a + b)
    print("resumed with x =", x)
    local y = coroutine.yield(x * 2)
    print("resumed with y =", y)
    return "done"
end)

print(coroutine.resume(co, 1, 2))
print(coroutine.resume(co, 10))
print(coroutine.resume(co, 20))
print("status:", coroutine.status(co))

-- coroutine.wrap
local gen = coroutine.wrap(function()
    for i = 1, 3 do
        coroutine.yield(i * i)
    end
end)
for i = 1, 3 do
    io.write(gen() .. " ")
end
print()

--------------------------------------------------------------------
-- 22. To-be-closed variables (Lua 5.4)
--------------------------------------------------------------------
print("\n-- To-be-closed variables (5.4) --")
do
    local function newResource(name)
        return setmetatable({name = name}, {
            __close = function(self) print("Closing resource:", self.name) end
        })
    end
    local ok5 = pcall(function()
        local res <close> = newResource("file_handle")
        print("Using resource:", res.name)
    end)
    if not ok5 then
        print("(<close> attribute not supported in this Lua version)")
    end
end

--------------------------------------------------------------------
-- 23. const variables (Lua 5.4)
--------------------------------------------------------------------
do
    local ok6 = pcall(load([[
        local PI <const> = 3.14159
        print("const PI =", PI)
    ]]))
    if not ok6 then
        print("(<const> attribute not supported in this Lua version)")
    end
end

--------------------------------------------------------------------
-- 24. os / io basics (safe, no external input)
--------------------------------------------------------------------
print("\n-- os / io --")
print("os.time() =", os.time())
print("os.date():", os.date("%Y-%m-%d"))
print("os.clock() =", os.clock())
io.write("io.write example\n")

--------------------------------------------------------------------
-- 25. Type checking & conversions
--------------------------------------------------------------------
print("\n-- Type checks & conversions --")
print(type(1), type("s"), type({}), type(print), type(nil), type(true))
print(tonumber("42"), tonumber("3.14"), tonumber("ff", 16))
print(tostring(123), tostring(nil), tostring(true))

--------------------------------------------------------------------
-- 26. rawget / rawset / rawequal / rawlen
--------------------------------------------------------------------
print("\n-- raw operations --")
local rt = setmetatable({}, {__index = function() return "default" end})
print("rt.missing (via metamethod) =", rt.missing)
print("rawget(rt, 'missing') =", rawget(rt, "missing"))
rawset(rt, "x", 99)
print("rt.x =", rt.x)
print("rawequal(1, 1.0) =", rawequal(1, 1.0))
print("rawlen({1,2,3}) =", rawlen({1, 2, 3}))

--------------------------------------------------------------------
-- 27. Weak tables (for GC-related tests)
--------------------------------------------------------------------
print("\n-- Weak tables --")
local weak = setmetatable({}, {__mode = "k"})
local key = {}
weak[key] = "value tied to key"
print("weak table entry:", weak[key])

--------------------------------------------------------------------
-- 28. Custom iterators (generic for)
--------------------------------------------------------------------
print("\n-- Custom iterator --")
local function range(n)
    local i = 0
    return function()
        i = i + 1
        if i <= n then return i end
    end
end
for v in range(5) do
    io.write(v .. " ")
end
print()

--------------------------------------------------------------------
-- 29. Stateless iterator using generic for with explicit state
--------------------------------------------------------------------
local function iter(t, i)
    i = i + 1
    local v = t[i]
    if v then return i, v end
end
local function myIpairs(t)
    return iter, t, 0
end
print("\n-- Stateless iterator --")
for i, v in myIpairs({"x", "y", "z"}) do
    print(i, v)
end

print("\nAll extended syntax examples executed.")

if a then
    while b do
        repeat
            for i=1,10 do
                if c then
                    do
                        local x = function(...)
                            return function()
                                return {
                                    a = {
                                        b = {
                                            c = {}
                                        }
                                    }
                                }
                            end
                        end
                    end
                end
            end
        until d
    end
end

((((((((42))))))))

((((function() end))))

(function() end)

(function() return 1 end)()

((((print))))("hello")

a.b.c.d.e

((((f))))()

-- cant parse ts bruh
-- a[b][c][d]
-- a():b():c()
-- a().b.c()
-- ((((a))))().b[1]:c().d
-- f{}
-- f"hello"
-- f[[abc]]
-- f[=[abc]=]
