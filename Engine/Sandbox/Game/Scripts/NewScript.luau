NewScript = { }

function NewScript:OnAttach()
end

function NewScript:OnReady()
    self.CC  = SCharacterControllerComponent.Get(self.Registry, self.Entity)
    self.CMC = SCharacterMovementComponent.Get(self.Registry, self.Entity)
    self.T   = STransformComponent.Get(self.Registry, self.Entity)
end


function NewScript:Update(DeltaTime: number)
    local x, z = 0.0, 0.0

    for i = 1, 100 do
    World:NewEntity()
    end

    if Input.IsKeyDown(EKey.W) then z =  1.0 end
    if Input.IsKeyDown(EKey.S) then z = -1.0 end
    if Input.IsKeyDown(EKey.A) then x =  1.0 end
    if Input.IsKeyDown(EKey.D) then x = -1.0 end

    if x ~= 0.0 or z ~= 0.0 then
        self.CC:AddMovementInput(vector.create(x, 0.0, z))
    end
end

return NewScript
