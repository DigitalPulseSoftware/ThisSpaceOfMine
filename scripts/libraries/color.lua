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
Color.Blue = Color(0.0, 0.0, 1.0)
Color.Cyan = Color(0.0, 1.0, 1.0)
Color.DarkBlue = Color(0.0, 0.0, 139.0 / 255.0)
Color.DarkGreen = Color(0.0, 100.0 / 255.0, 0.0)
Color.DarkRed = Color(139.0 / 255.0, 0.0, 0.0)
Color.Gray = Color(80.0 / 255.0, 80.0 / 255.0, 80.0 / 255.0)
Color.Green = Color(0.0, 1.0, 0.0)
Color.Magenta = Color(1.0, 0.0, 1.0)
Color.Orange = Color(1.0, 165.0 / 255.0, 0.0)
Color.Red = Color(1.0, 0.0, 0.0)
Color.Yellow = Color(1.0, 1.0, 0.0)
Color.White = Color(1.0, 1.0, 1.0)
