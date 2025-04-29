return function ()
    local playerEntity = CurrentPlayer:GetEntity()
    if not playerEntity then
        print("player has no entity")
        return
    end

    local playerPhys = playerEntity:GetComponent("physicscharacter3d")
    local layer = playerPhys:GetObjectLayer()
    if layer == Constants.ObjectLayerPlayer then
        playerPhys:SetObjectLayer(Constants.ObjectLayerPlayerOnlyTrigger)
        print("noclip enabled")
    else
        playerPhys:SetObjectLayer(Constants.ObjectLayerPlayer)
        print("noclip disabled")
    end
end
