local QuaternionMt = CreateMetatable("quaternion")
QuaternionMt.__index = QuaternionMt

function QuaternionMt:GetConjugate()
    return Quaternion(self.w, -self.x, -self.y, -self.z)
end

function QuaternionMt:GetLength()
    return math.sqrt(self.w * self.w + self.x * self.x + self.y * self.y + self.z * self.z)
end

function QuaternionMt:Normalize()
    local length = self:GetLength()
    self.w = self.w / length
    self.x = self.x / length
    self.y = self.y / length
    self.z = self.z / length
    return self
end

function QuaternionMt:ToDirection()
    return self * Vec3.Forward
end

function QuaternionMt:__mul(quat)
    local mt = getmetatable(quat)
    if mt == QuaternionMt then
        return Quaternion(
            self.w * quat.w - self.x * quat.x - self.y * quat.y - self.z * quat.z,
            self.w * quat.x + self.x * quat.w + self.y * quat.z - self.z * quat.y,
            self.w * quat.y + self.y * quat.w + self.z * quat.x - self.x * quat.z,
            self.w * quat.z + self.z * quat.w + self.x * quat.y - self.y * quat.x
        )
    else
        local quatVec = Vec3(self.x, self.y, self.z)
        local uv = quatVec:CrossProduct(quat)
        local uuv = quatVec:CrossProduct(uv)

        uv = uv * 2.0 * self.w
        uuv = uuv * 2.0

        return quat + uv + uuv
    end
end

function QuaternionMt:__tostring()
    return string.format("Quaternion(%f | %f, %f, %f)", self.w, self.x, self.y, self.z)
end

local QuaternionClassMt = {}

function QuaternionClassMt.__call(t, w, x, y, z)
    return setmetatable({w = w, x = x, y = y, z = z}, QuaternionMt)
end

Quaternion = {}
Quaternion.Metatable = QuaternionMt

function Quaternion.CombineRotations(...)
    local count = select("#", ...)
    local rotation = select(-1, ...)
    for i = count - 1, 1, -1 do
        rotation = rotation * select(i, ...)
    end
    return rotation:Normalize()
end

setmetatable(Quaternion, QuaternionClassMt)

Quaternion.Identity = Quaternion(1, 0, 0, 0)
