local BoxMt = CreateMetatable("box")
BoxMt.__index = BoxMt

function BoxMt:GetCenter()
    return self:GetPosition() + self:GetLengths() * 0.5
end

function BoxMt:GetMaximum()
    return self:GetPosition() + self:GetLengths()
end

function BoxMt:GetMinimum()
    return self:GetPosition()
end

function BoxMt:GetPosition()
    return Vec3(self.x, self.y, self.z)
end

function BoxMt:GetLengths()
    return Vec3(self.width, self.height, self.depth)
end

function BoxMt:Translate(offset)
    self.x = self.x + offset.x
    self.y = self.y + offset.y
    self.z = self.z + offset.z
end

function BoxMt:__tostring()
    return string.format("Box(%f, %f, %f, %f, %f, %f)", self.x, self.y, self.z, self.width, self.height, self.depth)
end

local BoxClassMt = {}

function BoxClassMt.__call(t, x, y, z, width, height, depth)
    return setmetatable({
        x = x,
        y = y, 
        z = z,
        width = width,
        height = height,
        depth = depth
    }, BoxMt)
end

Box = {}
Box.Metatable = BoxMt

function Box.FromExtents(vec1, vec2)
    local x = math.min(vec1.x, vec2.x)
    local y = math.min(vec1.y, vec2.y)
    local z = math.min(vec1.z, vec2.z)
    local width = vec2.x > vec1.x and vec2.x - vec1.x or vec1.x - vec2.x
    local height = vec2.y > vec1.y and vec2.y - vec1.y or vec1.y - vec2.y
    local depth = vec2.z > vec1.z and vec2.z - vec1.z or vec1.z - vec2.z
    return Box(x, y, z, width, height, depth)
end

setmetatable(Box, BoxClassMt)
