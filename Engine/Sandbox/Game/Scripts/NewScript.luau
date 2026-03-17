local NewScript = {
}

local CharacterController: SCharacterControllerComponent?

function NewScript:OnAttach()
end

function NewScript:OnReady()

    self.Registry = World:GetEntityRegistry()
end


function NewScript:Update(DeltaTime: number)
    local x, z = 0.0, 0.0

    local Value = Input.GetMouseDeltaX()
    Input.IsKeyDown(EKey.W)
    print(self.Registry:Has(self.Entity, STransformComponent))
    self.Cha

end

return NewScript
