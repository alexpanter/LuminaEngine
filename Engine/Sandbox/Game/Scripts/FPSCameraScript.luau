
NewScript = { }

local PlayerEntity = 1048580
local Registry = nil

function NewScript:OnAttach()
    -- Called when the entity is created
end

function NewScript:OnReady()
    Registry = World:GetEntityRegistry()
end

function NewScript:Update(DeltaTime: number)
    local PlayerTransform = Registry:Get(PlayerEntity, STransformComponent)

    self.Transform:SetLocation(PlayerTransform:GetLocation() + vector.create(0.0, 1.0, 0.0))
    
    print(PlayerTransform:GetRotation().Y)

    local MouseX = Input.GetMouseX()
    local MouseY = Input.GetMouseY()
    
    

    World:MarkTransformDirty(self.Entity)

end
            
function NewScript:OnDetach()
    -- Called when the entity is destroyed.
end


return NewScript
