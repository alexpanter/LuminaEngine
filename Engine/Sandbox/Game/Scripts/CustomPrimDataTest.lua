-- New Lua Script

CustomPrimDataTest = {

}


function CustomPrimDataTest:Update()
    local Mesh = Context:Get(Entity, SStaticMeshComponent)

    if Mesh then
        Mesh.CustomPrimitiveData:SetAsFloat(math.sin(Context:GetTime()))
    end
end


return CustomPrimDataTest;