
local Vec3Mt = CreateMetatable("vec3")
Vec3Mt.__index = Vec3Mt

function Vec3Mt:CrossProduct(vec)
    return Vec3(self.y * vec.z - self.z * vec.y, self.z * vec.x - self.x * vec.z, self.x * vec.y - self.y * vec.x)
end

function Vec3Mt:DotProduct(vec)
    return self.x * vec.x + self.y * vec.y + self.z * vec.z
end

function Vec3Mt:GetAbs()
    return Vec3(math.abs(self.x), math.abs(self.y), math.abs(self.z))
end

function Vec3Mt:GetLength()
    return math.sqrt(self.x * self.x + self.y * self.y + self.z * self.z)
end

function Vec3Mt:GetNormal()
    local length = self:GetLength()
    return Vec3(self.x / length, self.y / length, self.z / length), length
end

function Vec3Mt:Maximize(vec)
    return Vec3(math.max(self.x, vec.x), math.max(self.y, vec.y), math.max(self.z, vec.z))
end

function Vec3Mt:Minimize(vec)
    return Vec3(math.min(self.x, vec.x), math.min(self.y, vec.y), math.min(self.z, vec.z))
end

function Vec3Mt:__add(vec)
    return Vec3(self.x + vec.x, self.y + vec.y, self.z + vec.z)
end

function Vec3Mt:__sub(vec)
    return Vec3(self.x - vec.x, self.y - vec.y, self.z - vec.z)
end

function Vec3Mt:__mul(vec)
    if type(vec) == "number" then
        return Vec3(self.x * vec, self.y * vec, self.z * vec)
    else
        return Vec3(self.x * vec.x, self.y * vec.y, self.z * vec.z)
    end
end

function Vec3Mt:__div(vec)
    if type(vec) == "number" then
        return Vec3(self.x / vec, self.y / vec, self.z / vec)
    else
        return Vec3(self.x / vec.x, self.y / vec.y, self.z / vec.z)
    end
end

function Vec3Mt:__unm()
    return Vec3(-self.x, -self.y, -self.z)
end

function Vec3Mt:__tostring()
    return string.format("Vec3(%f, %f, %f)", self.x, self.y, self.z)
end

local Vec3ClassMt = {}

function Vec3ClassMt.__call(t, x, y, z)
    return setmetatable({x = x, y = y or x, z = z or x}, Vec3Mt)
end

Vec3 = {}
Vec3.Metatable = Vec3Mt

function Vec3.Distance(vec1, vec2)
    return (vec2 - vec1):GetLength()
end

setmetatable(Vec3, Vec3ClassMt)
