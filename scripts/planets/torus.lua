local perlin = PerlinNoise()
local chunksize = 32
local scale = 0.02-- / 4
local freespace = 30

-- https://iquilezles.org/articles/distfunctions/
local function sdTorus(p, t)
    local q = Vec2(Vec2(p.x, p.z):GetLength() - t.x, p.y)
    return q:GetLength() - t.y
end

local function sdOctahedron(p, s)
    p = Vec3(math.abs(p.x), math.abs(p.y), math.abs(p.z))
    return (p.x+p.y+p.z-s)*0.57735027;
end

--[[
float sdCappedCylinder( vec3 p, float r, float h )
{
  vec2 d = abs(vec2(length(p.xz),p.y)) - vec2(r,h);
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}
]]

local function sdCappedCylinder(p, r, h)
  local d = Vec2(Vec2(p.x, p.z):GetLength(), math.abs(p.y)) - Vec2(r, h)
  d.x = math.max(d.x, 0.0)
  d.y = math.max(d.y, 0.0)
  return math.min(math.max(d.x, d.y),0.0) + d:GetLength()
end


return function (chunk, seed, chunkcount)
    perlin:reseed(seed)
    math.randomseed(seed)

    local blockLibrary = chunk:GetBlockLibrary()
    local blockCount = chunk:GetBlockCount()

    local empty = blockLibrary:GetBlockIndex("empty")
    local dirt = blockLibrary:GetBlockIndex("dirt")
    local grass = blockLibrary:GetBlockIndex("grass")
    local snow = blockLibrary:GetBlockIndex("snow")
    local stone = blockLibrary:GetBlockIndex("stone")
    local stoneMossy = blockLibrary:GetBlockIndex("stone_mossy")

    local planet = chunk:GetContainer()
    local chunkIndices = chunk:GetIndices()

    local maxHeight = (Vec3(chunkcount.x, chunkcount.y, chunkcount.z) + Vec3(1)) / 2
    maxHeight = maxHeight * chunksize

    local content = table.new(chunksize * chunksize * chunksize, 0)
    
    for z = 0, chunksize - 1 do
        for y = 0, chunksize - 1 do
            for x = 0, chunksize - 1 do
                local blockPos = planet:GetBlockIndices(chunkIndices, Vec3(x, y, z))
                --[[local depth = math.min(
                    maxHeight.x - math.abs(blockPos.x),
                    maxHeight.y - math.abs(blockPos.y),
                    maxHeight.z - math.abs(blockPos.z)
                )]]
               --[[local depth = maxHeight.x - math.max(0, sdTorus(blockPos, Vec2(20, 50)))

                if (depth < freespace) then
                    table.insert(content, empty)
                    goto continue
                end

                depth = depth - freespace
                --depth = depth * 0.25

                local presence = perlin:normalizedOctave3D_01(blockPos.x * scale, blockPos.y * scale, blockPos.z * scale, 4, 0.5)
                if depth < 20 then
                    presence = presence * math.max(depth / 20.0, 1.0)
                end

                presence = presence + depth / math.max(maxHeight.x, maxHeight.y, maxHeight.z)

                local blockIndex
                if presence > 0.6 then
                    if depth < 6 * 2 then
                        blockIndex = snow
                    elseif depth <= 18 * 2 then
                        blockIndex = dirt
                    else
                        blockIndex = math.random() > 0.1 and stone or stoneMossy
                    end
                else
                    blockIndex = empty
                end

                table.insert(content, blockIndex)]]

                local dist = sdTorus(blockPos, Vec2(80, 20))
                if dist < 0 then
                    table.insert(content, dirt)
                else
                    table.insert(content, empty)
                end

                ::continue::
            end
        end
    end

    return content
end
