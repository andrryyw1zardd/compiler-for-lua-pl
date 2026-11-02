local Vector = {}
print(Vector.n)
print(Vector.x)
print(Vector.y)
print(Vector.foo)

local a = Vector.x + Vector.y

function(a, b)
    return Vector.new(
        a.x + b.x,
        a.y + b.y
    ) 
end

Vector.__add = function(a, b) return Vector.new(a.x + b.x, a.y + b.y) end
