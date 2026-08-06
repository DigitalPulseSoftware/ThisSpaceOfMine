function math.clamp(val, min, max)
    return math.max(math.min(val, max), min)
end

function math.easeInCubic(value)
    return value * value * value
end
