

local PlayerCameraController = {
    YOffset = 2,
    HeadBobPhase = 0.0
}

local HeadBobOffset = 0.0
local CameraTransformComponent = nil
local PlayerTransformComponent = nil
local PlayerControllerComponent = nil
local PlayerMovementComponent = nil

function PlayerCameraController:OnReady()
    CameraTransformComponent = Context:Get(Entity, STransformComponent)
end

function PlayerCameraController:Update()

    if not PlayerTransformComponent then
        local FollowPlayer = Context:GetByTag("Player")
        if FollowPlayer == entt.null then
            return
        end

        PlayerTransformComponent    = Context:Get(FollowPlayer, STransformComponent)
        PlayerControllerComponent   = Context:Get(FollowPlayer, SCharacterControllerComponent)
        PlayerMovementComponent     = Context:Get(FollowPlayer, SCharacterMovementComponent)
    end

    self.HeadBobPhase = self.HeadBobPhase or 0.0
    local VelocityLength = PlayerMovementComponent.Velocity:Length()
    if VelocityLength > 0.0 and PlayerMovementComponent.bGrounded then
        self.HeadBobPhase = self.HeadBobPhase + VelocityLength * Context:GetDeltaTime() * 1.2
        HeadBobOffset = math.sin(self.HeadBobPhase) * 0.12
    else
        self.HeadBobPhase = 0.0
        HeadBobOffset = 0.0
    end

    CameraTransformComponent:SetLocation(PlayerTransformComponent:GetLocation() + vec3(0, self.YOffset + HeadBobOffset, 0))

    local Pitch = PlayerControllerComponent.LookInput.y
    local Yaw   = PlayerControllerComponent.LookInput.x
    local NewRotation = vec3(Pitch, Yaw, 0.0)
    CameraTransformComponent:SetRotationFromEuler(NewRotation)

    Context:DirtyTransform(Entity)

end

return PlayerCameraController