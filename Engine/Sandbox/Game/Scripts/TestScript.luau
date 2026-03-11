local PlayerScript = {
    Test = 12
}

function PlayerScript:Update(DeltaTime: number)
    print(Input.IsKeyDown(EKey.W))
end

function PlayerScript:OnDetach()
end

return PlayerScript