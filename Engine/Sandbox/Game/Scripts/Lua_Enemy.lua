local EnemyController = {

}

local PlayerEntity = nil
local PlayerTransformComponent = nil
local TransformComponent = nil
local CharacterControllerComponent = nil
local StaticMeshComponent = nil
local HealthComponent = nil


function EnemyController:OnReady()
    TransformComponent              = Context:Get(Entity, STransformComponent)
    CharacterControllerComponent    = Context:Get(Entity, SCharacterControllerComponent)
    StaticMeshComponent             = Context:Get(Entity, SStaticMeshComponent)
    HealthComponent                 = Context:Get(Entity, SHealthComponent)
end

function EnemyController:Update()

    if not PlayerEntity then
        PlayerEntity = Context:GetByTag("Player")
        PlayerTransformComponent = Context:Get(PlayerEntity, STransformComponent)
    end

    local SelfLocation = TransformComponent:GetLocation()
    local PlayerLocation = PlayerTransformComponent:GetLocation()
    
    local Forward = glm.Normalize(PlayerLocation - SelfLocation)
    
    CharacterControllerComponent:AddMovementInput(Forward)

    StaticMeshComponent.CustomPrimitiveData:SetAsFloat(HealthComponent.Health * 0.01)

    if HealthComponent.Health <= 0.0 then
        Context:Destroy(Entity)
    end
end


function EnemyController:OnDetach()
    
end


return EnemyController