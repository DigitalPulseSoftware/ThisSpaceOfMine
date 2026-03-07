return function ()
    local playerEntity = CurrentPlayer:GetEntity()
    if not playerEntity then
        print("player has no entity")
        return
    end

    local playerExchanger = playerEntity:GetComponent("atmosphere_exchanger")
    if playerExchanger:GetTickRate() ~= Time.Zero() then
        playerExchanger:SetTickRate(Time.Zero())
        print("infinite breathe enabled")
    else
        playerExchanger:SetTickRate(Time.Second())
        print("infinite breathe disabled")
    end
end
