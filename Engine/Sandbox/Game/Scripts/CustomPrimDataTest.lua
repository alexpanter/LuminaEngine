-- New Lua Script

CustomPrimDataTest = {

}


function CustomPrimDataTest:Update()
    local Mesh = Context:Get(Entity, SStaticMeshComponent)

    if Mesh then
        Mesh.CustomPrimitiveData:SetAsFloat4(Color.Blue * 255.0)
    end
end


return CustomPrimDataTest;