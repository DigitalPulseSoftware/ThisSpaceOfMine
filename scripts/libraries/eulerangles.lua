local EulerAnglesMt = CreateMetatable("eulerangles")
EulerAnglesMt.__index = EulerAnglesMt

function EulerAnglesMt:ToQuaternion()
    local s1, c1 = math.sin(self.yaw * 0.5), math.cos(self.yaw * 0.5)
    local s2, c2 = math.sin(self.roll * 0.5), math.cos(self.roll * 0.5)
    local s3, c3 = math.sin(self.pitch * 0.5), math.cos(self.pitch * 0.5)

    return Quaternion(s1 * s2 * c3 + c1 * c2 * s3,
                      s1 * c2 * c3 + c1 * s2 * s3,
                      c1 * s2 * c3 - s1 * c2 * s3,
                      c1 * c2 * c3 - s1 * s2 * s3)
end

function EulerAngles(pitch, yaw, roll)
    return setmetatable({
        pitch = pitch,
        yaw = yaw, 
        roll = roll
    }, EulerAnglesMt)
end
