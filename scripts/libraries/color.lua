local ColorMt = CreateMetatable("color")
ColorMt.__index = ColorMt

function ColorMt:__mul(scale)
    return Color(self.r * scale, self.g * scale, self.b * scale)
end

function ColorMt:__tostring()
    return string.format("Color(%f, %f, %f, %f)", self.r, self.g, self.b, self.a)
end

local ColorClassMt = {}

function ColorClassMt.__call(t, r, g, b, a)
    return setmetatable({
        r = r,
        g = g,
        b = b,
        a = a or 1.0
    }, ColorMt)
end

Color = {}
Color.Metatable = ColorMt

setmetatable(Color, ColorClassMt)

Color.Black = Color(0.0, 0.0, 0.0)
Color.White = Color(1.0, 1.0, 1.0)
