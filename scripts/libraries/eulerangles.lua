
local EulerAnglesMt = CreateMetatable("eulerangles")
EulerAnglesMt.__index = EulerAnglesMt

function Color(pitch, yaw, roll)
    return setmetatable({
        pitch = pitch,
        yaw = yaw, 
        roll = roll
    }, EulerAnglesMt)
end
