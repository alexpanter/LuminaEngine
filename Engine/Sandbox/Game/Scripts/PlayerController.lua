-- New Lua Script

PlayerScript = {

}

function PlayerScript:OnReady()
    Input.SetMouseMode(EMouseMode.Captured)
end


function PlayerScript:Update(DeltaTime)

    local CharacterController = Context:Get(Entity, SCharacterControllerComponent)
    local CharacterMovement = Context:Get(Entity, SCharacterMovementComponent)

    
    if Input.IsKeyDown(EKey.W) then
        CharacterController:AddMovementInput(vec3(0, 0, 1))
    end

    if Input.IsKeyDown(EKey.S) then
        CharacterController:AddMovementInput(vec3(0, 0, -1))
    end

    if Input.IsKeyDown(EKey.A) then
        CharacterController:AddMovementInput(vec3(1, 0, 0))
    end

    if Input.IsKeyDown(EKey.D) then
        CharacterController:AddMovementInput(vec3(-1, 0, 0))
    end

    if Input.IsKeyDown(EKey.Space) then
        CharacterController:Jump()
    end

    if Input.IsKeyDown(EKey.LeftShift) then
        CharacterMovement.MoveSpeed = 15
    else
        CharacterMovement.MoveSpeed = 5
    end

    Context:DirtyTransform(Entity)

end


return PlayerScript