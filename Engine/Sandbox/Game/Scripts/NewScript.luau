

NewScript = { }
function NewScript:OnAttach()
end

function NewScript:OnReady()
    --self.Entity
    --self.Registry
end

function NewScript:Update(DeltaTime: number)
    print(DeltaTime)
end
            
function NewScript:OnDetach()
end

return NewScript
