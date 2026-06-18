-- Basic Lua Examples

-- 1. Variables and Data Types
local name = "Lua"
local version = 5.4
local isAwesome = true

print("Language: " .. name)
print("Version: " .. version)
print("Awesome: " .. tostring(isAwesome))

-- 2. Control Flow
local score = 85

if score >= 90 then
    print("Grade: A")
elseif score >= 80 then
    print("Grade: B")
elseif score >= 70 then
    print("Grade: C")
else
    print("Grade: F")
end

-- 3. Loops
print("\n-- For Loop --")
for i = 1, 5 do
    io.write(i .. " ")
end
print()

print("\n-- While Loop --")
local count = 0
while count < 3 do
    count = count + 1
    print("Count: " .. count)
end

-- 4. Functions
local function greet(person)
    return "Hello, " .. person .. "!"
end

print("\n" .. greet("World"))

-- Multiple return values
local function minMax(a, b)
    return math.min(a, b), math.max(a, b)
end

local lo, hi = minMax(-10, 3)
print("Min: " .. lo .. ", Max: " .. hi)

-- 5. Tables (arrays + dictionaries)
local fruits = {"apple", "banana", "cherry"}
for i, fruit in ipairs(fruits) do
    print(i .. ": " .. fruit)
end

local person = {name = "Alice", age = 30, city = "Paris"}
for key, value in pairs(person) do
    print(key .. " = " .. tostring(value))
end

-- 6. Simple OOP with metatables
local Animal = {}
Animal.__index = Animal

function Animal.new(name, sound)
    return setmetatable({name = name, sound = sound}, Animal)
end

function Animal:speak()
    print(self.name .. " says " .. self.sound)
end

local dog = Animal.new("Dog", "Woof!")
local cat = Animal.new("Cat", "Meow!")

dog:speak()
cat:speak()
