-- Do not use these example/testing scripts as a reference. They will have bad habbits.

local NewScript = {}

local CharacterControllerComponent: SCharacterControllerComponent?
local CharacterPhysicsComponent: SCharacterPhysicsComponent?
local CharacterMovementComponent: SCharacterMovementComponent?

function NewScript:OnAttach()
end

function NewScript:OnReady()
    CharacterControllerComponent    = World:GetEntityRegistry():Get(self.Entity, SCharacterControllerComponent)
    CharacterPhysicsComponent       = World:GetEntityRegistry():Get(self.Entity, SCharacterPhysicsComponent)
    CharacterMovementComponent      = World:GetEntityRegistry():Get(self.Entity, SCharacterMovementComponent)
end

function NewScript:Update(DeltaTime: number)

    local X: number = 0
    local Y: number = 0

    if Input.IsKeyDown(EKey.W) then Y += 1.0 end
    if Input.IsKeyDown(EKey.S) then Y -= 1.0 end
    if Input.IsKeyDown(EKey.A) then X += 1.0 end
    if Input.IsKeyDown(EKey.D) then X -= 1.0 end

    local MovementInput = vector.create(X, 0.0, Y)
    CharacterControllerComponent:AddMovementInput(MovementInput)


    local MouseX = Input.GetMouseDeltaX()
    local MouseY = Input.GetMouseDeltaY()

    CharacterControllerComponent:AddLookInput(vector.create(MouseX, MouseY))


end

function NewScript:OnDetach()
end

return NewScript
