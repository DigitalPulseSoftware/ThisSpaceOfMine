local QuaternionMt = CreateMetatable("quaternion")
QuaternionMt.__index = QuaternionMt

function QuaternionMt:GetConjugate()
    return Quaternion(self.w, -self.x, -self.y, -self.z)
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

function Quaternion(w, x, y, z)
    return setmetatable({w = w, x = x, y = y, z = z}, QuaternionMt)
end
