
local Vec2Mt = CreateMetatable("vec2")
Vec2Mt.__index = Vec2Mt

function Vec2Mt:DotProduct(vec)
    return self.x * vec.x + self.y * vec.y
end

function Vec2Mt:GetAbs()
    return Vec2(math.abs(self.x), math.abs(self.y))
end

function Vec2Mt:GetLength()
    return math.sqrt(self.x * self.x + self.y * self.y)
end

function Vec2Mt:GetNormal()
    local length = self:GetLength()
    return Vec2(self.x / length, self.y / length), length
end

function Vec2Mt:Maximize(vec)
    return Vec2(math.max(self.x, vec.x), math.max(self.y, vec.y))
end

function Vec2Mt:Minimize(vec)
    return Vec2(math.min(self.x, vec.x), math.min(self.y, vec.y))
end

function Vec2Mt:__add(vec)
    return Vec2(self.x + vec.x, self.y + vec.y)
end

function Vec2Mt:__sub(vec)
    return Vec2(self.x - vec.x, self.y - vec.y)
end

function Vec2Mt:__mul(vec)
    if type(vec) == "number" then
        return Vec2(self.x * vec, self.y * vec)
    else
        return Vec2(self.x * vec.x, self.y * vec.y)
    end
end

function Vec2Mt:__div(vec)
    if type(vec) == "number" then
        return Vec2(self.x / vec, self.y / vec)
    else
        return Vec2(self.x / vec.x, self.y / vec.y)
    end
end

local Vec2ClassMt = {}

function Vec2ClassMt.__call(t, x, y)
    return setmetatable({x = x, y = y or x}, Vec2Mt)
end

Vec2 = {}

function Vec2.Distance(vec1, vec2)
    return (vec2 - vec1):GetLength()
end

setmetatable(Vec2, Vec2ClassMt)
