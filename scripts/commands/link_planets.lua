return function (opt)
    opt = opt or {}
    local position = opt.position
    if not position then
        local playerEntity = CurrentPlayer:GetEntity()
        if not playerEntity then
            print("player has no entity")
            return
        end

        local playerNode = playerEntity:GetComponent("node")
        position = playerNode:GetPosition()
    end

    local linkData = {
        sourcePlanet = assert(opt.sourcePlanet or opt.src, "missing sourcePlanet parameter"),
        destinationPlanet = assert(opt.destinationPlanet or opt.dst, "missing destinationPlanet parameter"),
        position = assert(position, "missing position parameter")
    }

    if not opt.ephemeral then
        ServerDatabase.StorePlanetLink(linkData)
    end

    Server.LinkDatabaseEnvironments(linkData.sourcePlanet, linkData.destinationPlanet, linkData.position)

    if opt.dual then
        linkData.position = -linkData.position

        local temp = linkData.sourcePlanet
        linkData.sourcePlanet = linkData.destinationPlanet
        linkData.destinationPlanet = temp

        if not opt.ephemeral then
            ServerDatabase.StorePlanetLink(linkData)
        end

        Server.LinkDatabaseEnvironments(linkData.sourcePlanet, linkData.destinationPlanet, linkData.position)
    end

    print("link created")
end
