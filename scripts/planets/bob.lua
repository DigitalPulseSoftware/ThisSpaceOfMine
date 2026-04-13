-- Do not touch to this 2 variables
local perlin = PerlinNoise()
local cavePerlin = PerlinNoise()
local chunksize = 32

local minGrenerationFreeHeight = 0 -- Generation height limit used to make generation faster if we want empty chunks to allow players to build tall things
local baseFreeHeight = 40 -- Should be greater than minFreeHeight, difference between both will define max generation height from baseFreeHeight

local function GetBlockIndices(chunkIndices, blockIndices)
    local x = chunkIndices.x * chunksize + blockIndices.x - chunksize * 0.5
    local y = chunkIndices.y * chunksize + blockIndices.z - chunksize * 0.5
    local z = chunkIndices.z * chunksize + blockIndices.y - chunksize * 0.5
    return Vec3(x, y, z)
end

local function sdRoundBox(pos, dims, cornerRadius)
	local q = pos:GetAbs() - dims + Vec3(cornerRadius)
	return q:Maximize(Vec3(0, 0, 0)):GetLength() + math.min(math.max(q.x, math.max(q.y, q.z)), 0.0) - cornerRadius
end

return function (chunk, seed, chunkDims)
    perlin:reseed(seed)
    cavePerlin:reseed(seed * seed)

    local blockSize = chunk:GetBlockSize()

    local blockLibrary = chunk:GetBlockLibrary()
    local blockCount = chunk:GetBlockCount()

    local emptyBlock = blockLibrary:GetBlockIndex("empty")
    local debugBlock = blockLibrary:GetBlockIndex("debug")
    local dirtBlock = blockLibrary:GetBlockIndex("dirt")
    local grassBlock = blockLibrary:GetBlockIndex("grass")
    local hullBlock = blockLibrary:GetBlockIndex("hull")
    local snowBlock = blockLibrary:GetBlockIndex("snow")
    local stoneBlock = blockLibrary:GetBlockIndex("stone")
    local stoneMossyBlock = blockLibrary:GetBlockIndex("stone_mossy")
    local forcefieldBlock = blockLibrary:GetBlockIndex("forcefield")
    local planksBlock = blockLibrary:GetBlockIndex("planks")
    local stoneBricksBlock = blockLibrary:GetBlockIndex("stone_bricks")
    local goldBlock = blockLibrary:GetBlockIndex("gold")
    local glassBlock = blockLibrary:GetBlockIndex("glass")
    local waterBlock = blockLibrary:GetBlockIndex("water")
    local rockBlock = blockLibrary:GetBlockIndex("rock")
    local barkBlock = blockLibrary:GetBlockIndex("bark")
    local cliffRock = blockLibrary:GetBlockIndex("cliff_rocks")

    local planet = chunk:GetContainer()
    local chunkIndices = chunk:GetIndices()
    
    local maxHeight = (chunksize * chunkDims.x)/2 * blockSize
    local maxGenerationHeight = maxHeight - minGrenerationFreeHeight
    local baseHeight = maxHeight - baseFreeHeight -- Only works for planets with the same number of chunks in all the directions
    local caveBaseHeight = baseHeight - 12.0

    local terrainVariation1Scale = 0.06 * baseHeight
    local terrainVariation2Scale = 0.16 * baseHeight
    local moutainScale = 0.035 * baseHeight
    local spikeScale = 0.1 * baseHeight
    local smallCaveScale = 0.10  -- Other scale unit
    local bigCaveScale = 0.03  -- Other scale unit
    local stoneScale = 0.35 * baseHeight

    local content = table.new(chunksize * chunksize * chunksize, 0)

    for z = 0, chunksize - 1 do
        for y = 0, chunksize - 1 do
            for x = 0, chunksize - 1 do
                local blockPos = GetBlockIndices(chunkIndices, Vec3(x, y, z))
                local blockPosScaled = blockPos * blockSize
                local blockPosNorm, distToCenter = blockPosScaled:GetNormal()

                -- center of the planet
                if distToCenter < 16.0 then
                    table.insert(content, distToCenter < 2.0 and goldBlock or emptyBlock)
                    goto continue
                end

                --distToCenter = math.max(math.abs(blockPos.x * 0.5 + 0.5), math.abs(blockPos.y * 0.5 + 0.5), math.abs(blockPos.z * 0.5 + 0.5))
                distToCenter = sdRoundBox(blockPosScaled, Vec3(baseHeight), 32.0)

                local baseMountainous = perlin:normalizedOctave3D_01((blockPosNorm.x * moutainScale)+10, blockPosNorm.y * moutainScale, blockPosNorm.z * moutainScale, 4, 0.1)
                local mountainous
                if baseMountainous < 0.6 then 
                    mountainous = 0
                elseif baseMountainous < 0.8 then 
                    mountainous = 5*baseMountainous-3
                else
                    mountainous = 1
                end
                
                local heightVariation1 = 10 * perlin:normalizedOctave3D_01(blockPosNorm.x * terrainVariation1Scale, blockPosNorm.y * terrainVariation1Scale, blockPosNorm.z * terrainVariation1Scale, 4, 0.1)
                local heightVariation2 = 50 * mountainous * perlin:normalizedOctave3D_01((blockPosNorm.x * terrainVariation2Scale)+20, blockPosNorm.y * terrainVariation2Scale, blockPosNorm.z * terrainVariation2Scale, 4, 0.1)
                
                local baseSpikeHeight = perlin:normalizedOctave3D_01((blockPosNorm.x * spikeScale)+30, blockPosNorm.y * spikeScale, blockPosNorm.z * spikeScale, 4, 0.1)
                local spikeHeight
                if baseSpikeHeight < 0.7 then 
                    spikeHeight = 0
                elseif baseSpikeHeight < 0.9 then 
                    spikeHeight = 5*baseSpikeHeight-3.5
                else
                    spikeHeight = 1
                end
                spikeHeight = (1-mountainous) * spikeHeight * 15
                
                local height = heightVariation1 + heightVariation2 + spikeHeight
                
                local waterScale = 0.01 * baseHeight
                local waterNoise = perlin:normalizedOctave3D((blockPosNorm.x * waterScale)+10, blockPosNorm.y * waterScale, blockPosNorm.z * waterScale, 4, 0.1)

                local blockType = emptyBlock

                if waterNoise > 0.5 then
                    height = -5
                    if distToCenter < 0.0 then
                        blockType = waterBlock
                    end
                end

                if distToCenter <= height then
                    if distToCenter >= height - spikeHeight then
                        blockType = rockBlock
                    elseif mountainous > 0.5 and heightVariation2 > 0.5 then
                        blockType = snowBlock
                    elseif mountainous > 0.1 then
                        blockType = cliffRock
                    elseif baseMountainous < 0.6 then
                        local stoneNoise = perlin:normalizedOctave3D((blockPosNorm.x * stoneScale)+10, blockPosNorm.y * stoneScale, blockPosNorm.z * stoneScale, 4, 0.1)
                        blockType = stoneNoise >= 0.4 and stoneBlock or grassBlock
                    else
                        blockType = rockBlock
                    end
                    if distToCenter < 0.0 then
                        blockType = dirtBlock
                    end
                    table.insert(content, blockType)
                else
                    table.insert(content, blockType)
                end

                ::continue::
            end
        end
    end

    -- Caves
    for z = 0, chunksize - 1 do
        for y = 0, chunksize - 1 do
            for x = 0, chunksize - 1 do
                local blockPos = GetBlockIndices(chunkIndices, Vec3(x, y, z))
                local blockPosScaled = blockPos * blockSize
                local distToCenter = sdRoundBox(blockPosScaled, Vec3(caveBaseHeight), 32.0)

                local smallCavePresence = cavePerlin:normalizedOctave3D(blockPosScaled.x * smallCaveScale, blockPosScaled.y * smallCaveScale, blockPosScaled.z * smallCaveScale, 4, 0.1)
                local bigCavePresence = cavePerlin:normalizedOctave3D(blockPosScaled.x * bigCaveScale, blockPosScaled.y * bigCaveScale, blockPosScaled.z * bigCaveScale, 4, 0.1)

                local smallCaveMinValue = 0.5
                local bigCaveMinValue = 0.4
                if distToCenter > 16.0 then
                    smallCaveMinValue = smallCaveMinValue + distToCenter / 4.0 * 0.1
                end
                if distToCenter > 0.0 then
                    bigCavePresence = bigCavePresence * 2
                    bigCaveMinValue = bigCaveMinValue + distToCenter * 0.1
                end

                if math.abs(smallCavePresence) > smallCaveMinValue or math.abs(bigCavePresence) > bigCaveMinValue then
                    local blockIndex = z * chunksize * chunksize + y * chunksize + x + 1
                    if content[blockIndex] == waterBlock then
                        content[blockIndex] = dirtBlock
                    else
                        content[blockIndex] = emptyBlock
                    end
                end
            end
        end
    end

    return content
end
