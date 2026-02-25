

PlayerController = {

}

local CharacterController = nil
local MovementComponent = nil
local TransformComponent = nil
local BodyID = nil
local CubeMesh = nil


local function Shoot(Forward, Distance)

    local Projectile    = Context:Create(TransformComponent:GetLocation() + vec3(0, 2, 0) + Forward * 5)
    Context:Get(Projectile, STransformComponent):SetScale(vec3(0.15, 0.15, 0.15))

    local BoxCollider   = SBoxColliderComponent()
    
    BoxCollider.HalfExtent = vec3(1.0, 1.0, 1.0)

    local BoxMesh       = SStaticMeshComponent()
    BoxMesh:SetStaticMesh(CubeMesh)

    Context:Emplace(Projectile, BoxMesh)
    Context:Emplace(Projectile, BoxCollider)

    local RigidBody = Context:Get(Projectile, SRigidBodyComponent)

    local ImpulseEvent = SImpulseEvent()
    ImpulseEvent.Impulse = Forward * Distance * 5
    ImpulseEvent.BodyID = RigidBody.BodyID
    Context:DispatchEvent(ImpulseEvent)


    local RayEnd = Forward * Distance
    local HitResult = Context:CastRay(TransformComponent:GetLocation() + vec3(0, 2, 0), RayEnd, false, 1.0, 0, BodyID)
    if not HitResult then
        return
    end

    local HealthComponent = Context:Get(HitResult.Entity, SHealthComponent)
    if not HealthComponent then
        return
    end

    HealthComponent:ApplyDamage(0.5)
end

local function HandleMovement(Controller, MoveComp)
    local MovementInput = vec3(0, 0, 0)
    
    if Input.IsKeyDown(EKey.W) then
        MovementInput = MovementInput + vec3(0, 0, 1)
    end

    if Input.IsKeyDown(EKey.S) then
        MovementInput = MovementInput + vec3(0, 0, -1)
    end

    if Input.IsKeyDown(EKey.A) then
        MovementInput = MovementInput + vec3(1, 0, 0)
    end

    if Input.IsKeyDown(EKey.D) then
        MovementInput = MovementInput + vec3(-1, 0, 0)
    end

    if Input.IsKeyDown(EKey.Space) then
        Controller:Jump()
    end

    if Input.IsKeyDown(EKey.LeftShift) then
        MoveComp.MoveSpeed = 20.0
    else
        MoveComp.MoveSpeed = 10.0
    end

    Controller:AddMovementInput(MovementInput)

    if Input.IsMouseButtonDown(EMouseKey.ButtonRight) then
        local MouseDeltaX = Input.GetMouseDeltaX() * 0.05
        local MouseDeltaY = Input.GetMouseDeltaY() * 0.05
        Controller:AddLookInput(vec2(-MouseDeltaX, MouseDeltaY))
    end

    if Input.IsMouseButtonDown(EMouseKey.ButtonLeft) then

        local Pitch = Controller.LookInput.y
        local Yaw   = Controller.LookInput.x

        local CosPitch  = math.cos(math.rad(-Pitch))
        local SinPitch  = math.sin(math.rad(-Pitch))
        local CosYaw    = math.cos(math.rad(Yaw))
        local SinYaw    = math.sin(math.rad(Yaw))

        local Forward = vec3(CosPitch * SinYaw, SinPitch, CosPitch * CosYaw)
        
        Shoot(Forward, 300)
    end
end


function PlayerController:OnReady()
    CharacterController = Context:Get(Entity, SCharacterControllerComponent)
    MovementComponent   = Context:Get(Entity, SCharacterMovementComponent)
    TransformComponent  = Context:Get(Entity, STransformComponent)
    BodyID              = Context:Get(Entity, SCharacterPhysicsComponent):GetBodyID()
    CubeMesh            = LoadObject("/Engine/Resources/Content/Meshes/SM_Cube")
    --Input.SetMouseMode(EMouseMode.Captured)
    print("Foo")
end

function PlayerController:Update()
    if not CharacterController or not MovementComponent then
        return
    end

    HandleMovement(CharacterController, MovementComponent)
 

end


return PlayerController