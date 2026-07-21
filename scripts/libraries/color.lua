
local ColorMt = CreateMetatable("color")
ColorMt.__index = ColorMt

function ColorMt:GetCenter()
    return self:GetPosition() + self:GetLengths() * 0.5
end

function ColorMt:GetPosition()
    return Vec3(self.x, self.y, self.z)
end

function ColorMt:GetLengths()
    return Vec3(self.width, self.height, self.depth)
end

function ColorMt:__mul(scale)
    return Color(self.r * scale, self.g * scale, self.b * scale)
end

function Color(r, g, b, a)
    return setmetatable({
        r = r,
        g = g, 
        b = b,
        a = a or 1.0
    }, ColorMt)
end
