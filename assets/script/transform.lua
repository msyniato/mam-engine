 function init(e)
end
 
function update(e, dt)
    local t = ECS.getTransform(e)
    if t == nil then return end
 
    local r = t.rotation
    r.y = r.y + dt * 2.0
    t.rotation = r
 
    if Input.isKeyPressed(Key.W) then
        local p = t.position
        p.y = p.y + dt * 5.0
        t.position = p
    end
end
 
function destroy(e)
end