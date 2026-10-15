local a = 10
local b, c = 20, "Hello"

local obj = {field1}
obj.field1 = 10

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

-- new
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
print("7 // 2 =", "7" // 2)
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
--
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
