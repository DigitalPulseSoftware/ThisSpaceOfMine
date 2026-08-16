local EulerAnglesMt = CreateMetatable("eulerangles")
EulerAnglesMt.__index = EulerAnglesMt

function EulerAnglesMt:ToQuaternion()
    local pitch = math.rad(self.pitch)
    local yaw = math.rad(self.yaw)
    local roll = math.rad(self.roll)

    local s1, c1 = math.sin(yaw * 0.5), math.cos(yaw * 0.5)
    local s2, c2 = math.sin(roll * 0.5), math.cos(roll * 0.5)
    local s3, c3 = math.sin(pitch * 0.5), math.cos(pitch * 0.5)

    return Quaternion(c1 * c2 * c3 - s1 * s2 * s3,
                      s1 * s2 * c3 + c1 * c2 * s3,
                      s1 * c2 * c3 + c1 * s2 * s3,
                      c1 * s2 * c3 - s1 * c2 * s3)
end

function EulerAngles(pitch, yaw, roll)
    return setmetatable({
        pitch = pitch,
        yaw = yaw, 
        roll = roll
    }, EulerAnglesMt)
end
