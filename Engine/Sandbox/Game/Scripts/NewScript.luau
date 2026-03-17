-- Do not use these example/testing scripts as a reference. They will have bad habbits.

local NewScript = {
}

NewScript.__meta = {
    bRunInEditor = false,
}

function NewScript:OnAttach()
end

function NewScript:OnReady()
end

function NewScript:Update(DeltaTime: number)
end

function NewScript:OnDetach()
end

return NewScript
