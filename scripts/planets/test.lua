-- Do not touch to this 2 variables
local perlin = PerlinNoise()
local chunksize = 32

local minGrenerationFreeHeight = 0 -- Generation height limit used to make generation faster if we want empty chunks to allow players to build tall things
local baseFreeHeight = 30 -- Should be greater than minFreeHeight, difference between both will define max generation height from baseFreeHeight

local abs = math.abs
local max = math.max
local min = math.min

return function (chunk, seed, chunkDims)
    perlin:reseed(seed)

    local blockLibrary = chunk:GetBlockLibrary()
    local blockCount = chunk:GetBlockCount()

    local emptyBlock = blockLibrary:GetBlockIndex("empty")
    local debugBlock = blockLibrary:GetBlockIndex("debug")
    local dirtBlock = blockLibrary:GetBlockIndex("dirt")
    local grassBlock = blockLibrary:GetBlockIndex("grass")
    local hullBlock = blockLibrary:GetBlockIndex("hull")
    local hull2Block = blockLibrary:GetBlockIndex("hull2")
    local snowBlock = blockLibrary:GetBlockIndex("snow")
    local stoneBlock = blockLibrary:GetBlockIndex("stone")
    local stoneMossyBlock = blockLibrary:GetBlockIndex("stone_mossy")
    local forcefieldBlock = blockLibrary:GetBlockIndex("forcefield")
    local planksBlock = blockLibrary:GetBlockIndex("planks")
    local stoneBricksBlock = blockLibrary:GetBlockIndex("stone_bricks")
    local copperBlock = blockLibrary:GetBlockIndex("copper_block")
    local glassBlock = blockLibrary:GetBlockIndex("glass")

    local planet = chunk:GetContainer()
    local chunkIndices = chunk:GetIndices()
    
    local maxHeight = (chunksize * chunkDims.x)/2 / 4;
    local maxGenerationHeight = maxHeight - minGrenerationFreeHeight
    local baseHeight = maxHeight - baseFreeHeight -- Only works for planets with the same number of chunks in all the directions
    
    local terrainVariation1Scale = 0.06 * baseHeight
    local terrainVariation2Scale = 0.16 * baseHeight
    local moutainScale = 0.03 * baseHeight
    local spikeScale = 0.2 * baseHeight
    local caveScale = 0.06 / 2 -- Other scale unit
    
    local content = {}
    local roundBox = SignedDistance.RoundBox
    for z = 0, chunksize - 1 do
        for y = 0, chunksize - 1 do
            for x = 0, chunksize - 1 do
                local blockPos = planet:GetBlockIndices(chunkIndices, Vec3ui(x, y, z))
                local blockPosNorm, distToCenter = Vec3f(blockPos.x * 0.5, blockPos.y * 0.5, blockPos.z * 0.5):GetNormal()
                --distToCenter = math.max(math.abs(blockPos.x * 0.25 + 0.5), math.abs(blockPos.y * 0.25 + 0.5), math.abs(blockPos.z * 0.25 + 0.5))
                --distToCenter = roundBox(Vec3f(blockPos.x * 0.25, blockPos.z * 0.25, blockPos.y * 0.25), Vec3f(baseHeight, baseHeight, baseHeight), 16.0)

                if distToCenter > 60 then
                    table.insert(content, emptyBlock)
                else
                    table.insert(content, dirtBlock)
                end
            end
        end
    end
    
    chunk:Reset(content)
end
