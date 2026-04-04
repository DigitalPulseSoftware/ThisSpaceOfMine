
local BoxMt = CreateMetatable("box")
BoxMt.__index = BoxMt

function BoxMt:GetCenter()
    return self:GetPosition() + self:GetLengths() * 0.5
end

function BoxMt:GetPosition()
    return Vec3(self.x, self.y, self.z)
end

function BoxMt:GetLengths()
    return Vec3(self.width, self.height, self.depth)
end

function Box(x, y, z, width, height, depth)
    return setmetatable({
        x = x,
        y = y, 
        z = z,
        width = width,
        height = height,
        depth = depth
    }, BoxMt)
end
