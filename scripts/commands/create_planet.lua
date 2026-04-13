return function (opt)
    opt = opt or {}
    local planetData = {
        chunkCount = opt.chunkCount or Vec3ui(3, 3, 3),
        cornerRadius = opt.cornerRadius or 0.0,
        generatorName = opt.generatorName or "bob",
        gravity = opt.gravity or 9.81,
        seed = opt.seed or 0
    }

    local currentPlanet
    local currentPos
    if opt.link then
        local playerEntity = CurrentPlayer:GetEntity()
        if not playerEntity then
            print("player has no entity")
            return
        end

        currentPlanet = playerEntity:GetEnvironment()
        if currentPlanet:GetType() ~= EnvironmentType.Planet then
            print("current environment is not a planet, cannot link!")
            return
        end

        if not currentPlanet:GetDatabaseId() then
            print("current planet doesn't exist in database")
            return
        end

        local playerNode = playerEntity:GetComponent("node")
        currentPos = playerNode:GetPosition()
    end

    local id = ServerDatabase.CreatePlanet(planetData)

    local planetEnv = PlanetEnvironment.new(id, planetData.generatorName, planetData.seed, planetData.chunkCount, 1.0, planetData.cornerRadius)
    Server.RegisterDatabaseEnvironment(id, planetEnv)

    print("new planet id: " .. id)

    if opt.link then
        local linkData = {
            sourcePlanet = currentPlanet:GetDatabaseId(),
            destinationPlanet = id,
            position = currentPos
        }

        print("Store link 1")
        ServerDatabase.StorePlanetLink(linkData)
        
        -- reverse link
        print("Store link 2")
        ServerDatabase.StorePlanetLink({ sourcePlanet = linkData.destinationPlanet, destinationPlanet = linkData.sourcePlanet, position = -linkData.position })

        print("Server link database environments")
        Server.LinkDatabaseEnvironments(linkData.sourcePlanet, linkData.destinationPlanet, linkData.position)
        Server.LinkDatabaseEnvironments(linkData.destinationPlanet, linkData.sourcePlanet, -linkData.position)
    end
end
