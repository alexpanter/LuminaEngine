#pragma once
#include "PhysicsTypes.h"
#include "Core/Templates/Optional.h"
#include "Ray/RayCast.h"


namespace Lumina::Physics
{
    class IPhysicsScene
    {
    public:

        virtual ~IPhysicsScene() { }
        virtual void PreUpdate() = 0;
        virtual void Update(double DeltaTime) = 0;
        virtual void PostUpdate() = 0;
        virtual void Simulate() = 0;
        virtual void StopSimulate() = 0;
        
        virtual void DeactivateBody(uint32 BodyID) = 0;
        virtual void ActivateBody(uint32 BodyID) = 0;
        virtual void ChangeBodyMotionType(uint32 BodyID, EBodyType NewType) = 0;
        
        virtual TOptional<SRayResult> CastRay(const SRayCastSettings& Settings) = 0;
        virtual TVector<SRayResult> CastSphere(const SSphereCastSettings& Settings) = 0;
    };
}
