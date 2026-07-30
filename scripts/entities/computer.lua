local consumption = 50

local classData = EntityRegistry.ClassBuilder()
classData:Set("spawnable", true)
classData:Set("spawnable_model", "computer")
classData:Set("spawnable_collider", Vec3(0.75))

classData:AddProperty("active", { type = "bool", default = false, isNetworked = true })

classData:On("init", function (self)
	local physSettings = {
		kind = "static",
		mass = 0.0,
		collider = BoxCollider3D.new(Vec3(0.75)),
		objectLayer = Constants.ObjectLayerStatic
	}

	self:AddComponent("rigidbody3d", physSettings)

	self:SetInteractible(true)

	if SERVER then
		local distribution = self:AddComponent("distribution", {
			inputs = { DistributionType.Electrical },
			outputs = {}
		})

		self:SetTickInterval(500)
		self.UsingPlayer = nil
	else
		self:SetInteractibleText("Pilot")

		local model = AssetLibrary.GetModel("computer")

		local gfx = self:AddComponent("graphics")
		gfx:AttachRenderable(model, Constants.RenderMask3D)
	end
end)

if SERVER then
	classData:On("distribution", function (self)
		local distribution = self:GetComponent("distribution")
		if not self:GetProperty("active") then
			distribution:UpdateConsumptionValue(0, 0)
			return
		end

		local consumptionValue = distribution:GetConsumptionValue(0) -- Get consumption value to handle update delay
		distribution:UpdateConsumptionValue(0, Distribution.ToTickUnit(consumption))

		local electricity = distribution:GetDistributedValue(DistributionType.Electrical)

		if electricity < consumptionValue then
			-- Not enough electricity, eject pilot
			local shipEnv = self:GetEnvironment()
			local shipEntity = shipEnv:GetExteriorShipEntity()
			if self.UsingPlayer and self.UsingPlayer:IsValid() and self.UsingPlayer:GetControlledShipEntity() == shipEntity then
				self.UsingPlayer:ExitPiloting()
				self.UsingPlayer = nil
				self:UpdateProperty("active", false)
			end
		end
	end)

	classData:On("interact", function (self, player)
		local computerNode = self:GetComponent("node")
		local shipEnv = self:GetEnvironment()
		local shipEntity = shipEnv:GetExteriorShipEntity()

		if player:PilotShip(shipEnv:GetShipEntity(), shipEnv:GetExteriorShipEntity(), computerNode:GetRotation()) then
			if self.UsingPlayer and self.UsingPlayer:IsValid() and self.UsingPlayer:GetControlledShipEntity() == shipEntity then
				self.UsingPlayer:ExitPiloting()
			end
			self.UsingPlayer = player
			self:UpdateProperty("active", true)
		end
	end)

	classData:On("tick", function (self)
		local shipEnv = self:GetEnvironment()
		local shipEntity = shipEnv:GetExteriorShipEntity()

		if self.UsingPlayer and (not self.UsingPlayer:IsValid() or self.UsingPlayer:GetControlledShipEntity() ~= shipEntity) then
			self.UsingPlayer = nil
			self:UpdateProperty("active", false)
		end
	end)
end

EntityRegistry.RegisterClass("computer", classData)
