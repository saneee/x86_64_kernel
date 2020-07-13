local inserttable = table.insert
local concat = table.concat
function obj_dump(data, showMetatable, lastCount)
    local out = {}
    if type(data) ~= "table" then
        --Value
        if type(data) == "string" then
            inserttable(out,"\"".. data.. "\"")
        else
            inserttable(out,tostring(data))
        end
    else
        --Format
        local count = lastCount or 0
        count = count + 1
        inserttable(out,"{\n")
        --Metatable
        if showMetatable then
            for i = 1,count do inserttable(out,"\t") end
            local mt = getmetatable(data)
            inserttable(out,"\"__metatable\" = ")
            inserttable(out,obj_dump(mt, showMetatable, count))    -- 如果不想看到元表的元表，可将showMetatable处填nil
            inserttable(out,",\n")     --如果不想在元表后加逗号，可以删除这里的逗号
        end
        --Key
        for key,value in pairs(data) do
            for i = 1,count do inserttable(out,"\t") end
            if type(key) == "string" then
                inserttable(out,"\""..key.. "\" = ")
            elseif type(key) == "number" then
                inserttable(out,"[".. key.. "] = ")

            else
                inserttable(out,tostring(key))
            end
            if count < 10 then
                inserttable(out,obj_dump(value, showMetatable, count)) -- 如果不想看到子table的元表，可将showMetatable处填nil
            end
            inserttable(out,",\n")     --如果不想在table的每一个item后加逗号，可以删除这里的逗号
        end
        --Format
        for i = 1,lastCount or 0 do inserttable(out,"\t") end
        inserttable(out,"}")
    end
    --Format
    if not lastCount then
        inserttable(out,"\n")
    end
    return concat(out,'');
end
function show_dump(t)
    yaos.say(obj_dump(t))
end
yaos.show_dump = show_dump;
