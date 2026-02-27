-- New Lua Script

local PlayerScript = {

}

function PlayerScript:OnReady()
    Input.SetMouseMode(EMouseMode.Captured)
end


local MoveInputs = {
    [EKey.W] = vec3.new(0, 0, 1),
    [EKey.S] = vec3.new(0, 0, -1),
    [EKey.A] = vec3.new(1, 0, 0),
    [EKey.D] = vec3.new(-1, 0, 0),
}

function PlayerScript:Update()
    local CharacterController = Context:Get(Entity, SCharacterControllerComponent)
    local CharacterMovement = Context:Get(Entity, SCharacterMovementComponent)

    for key, direction in MoveInputs do
        if Input.IsKeyDown(key) then
            CharacterController:AddMovementInput(direction)
        end
    end

    if Input.IsKeyDown(EKey.Space) then
        CharacterController:Jump()
    end

    CharacterMovement.MoveSpeed = Input.IsKeyDown(EKey.LeftShift) and 15 or 5
end

return PlayerScript