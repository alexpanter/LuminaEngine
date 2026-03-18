
NewScript = { }


local CharacterControllerComponent: SCharacterControllerComponent?
local CharacterPhysicsComponent: SCharacterPhysicsComponent?
local CharacterMovementComponent: SCharacterMovementComponent?

local TargetEntity: number?
local TargetLocation: vector?

local function FindNewTarget()
    return World:GetEntityByTag("Player")
end

local function HasReachedTarget(Current)
    local Dist: vector = TargetLocation - Current
    return vector.magnitude(Dist) < 3.0
end

function NewScript:OnAttach()
    -- Called when the entity is created
end

function NewScript:OnReady()
    CharacterControllerComponent    = World:GetEntityRegistry():Get(self.Entity, SCharacterControllerComponent)
    CharacterPhysicsComponent       = World:GetEntityRegistry():Get(self.Entity, SCharacterPhysicsComponent)
    CharacterMovementComponent      = World:GetEntityRegistry():Get(self.Entity, SCharacterMovementComponent)

end

function NewScript:Update(DeltaTime: number)
    
    if TargetEntity == nil then
        TargetEntity = FindNewTarget()
    end

    TargetLocation = Registry:Get(TargetEntity, STransformComponent):GetLocation()
    local SelfLocation = self.Transform:GetLocation()

    if HasReachedTarget(SelfLocation) then
        TargetEntity = nil
        TargetLocation = nil
        return
    end

    local LookAtRot = Engine.Math.FindLookAtRotation(TargetLocation, SelfLocation)
    local Rot = Engine.Math.QuatForward(LookAtRot)

    CharacterControllerComponent:AddMovementInput(Rot)

end
            
function NewScript:OnDetach()
    -- Called when the entity is destroyed.
end



return NewScript
