-- New Lua Script

local PlayerScript = {

}

function PlayerScript:OnReady()
    Input.SetMouseMode(EMouseMode.Captured)
end


function PlayerScript:Update()

    local t = {}

    for i = 1, 1000 do
        t[i] = "string_" .. i
    end    
end

return PlayerScript