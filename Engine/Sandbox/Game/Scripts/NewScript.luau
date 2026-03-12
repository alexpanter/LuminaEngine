NewScript = { }

function NewScript:OnAttach()
end

function NewScript:OnReady()
    --self.Entity
    --self.Registry
end

function NewScript:Update(DeltaTime: number)

    local CC  = SCharacterControllerComponent.Get(self.Registry, self.Entity)
    local CMC = SCharacterMovementComponent.Get(self.Registry, self.Entity)
    local T   = STransformComponent.Get(self.Registry, self.Entity)

    local x, z = 0.0, 0.0

    if Input.IsKeyDown(EKey.W) then
        z = 1.0
    end
    
    if Input.IsKeyDown(EKey.S) then
        z = -1.0
    end

    if Input.IsKeyDown(EKey.A) then
        x = 1.0
    end

    if Input.IsKeyDown(EKey.D) then
        x = -1.0
    end

    local Dir = vector.create(x, 0.0, z)
    CC:AddMovementInput(Dir)
end
            
function NewScript:OnDetach()
end

return NewScript
