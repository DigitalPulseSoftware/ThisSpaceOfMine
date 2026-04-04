
local QuaternionMt = CreateMetatable("quaternion")
QuaternionMt.__index = QuaternionMt

function QuaternionMt:GetConjugate()
    return Quaternion(-self.x, -self.y, -self.z, self.w)
end

function QuaternionMt:__mul(quat)
    assert(type(quat) == "userdata")
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
        local uv = quatVec.CrossProduct(quat)
        local uuv = quatVec.CrossProduct(uv)

        uv = uv * 2.0 * self.w
        uuv = uuv * 2.0

        return quat + uv + uuv
    end
end

function Quaternion(x, y, z, w)
    return setmetatable({x = x, y = y, z = z, w = w}, QuaternionMt)
end
