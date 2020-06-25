local ttt={};
table.insert(ttt,1);
yaos.say("22222222222,test2.lua\n"..tostring({next=2}));
local nr = 0;
local function cb2(r)
    nr = nr + 1;
    yaos.say("test2.lua,NR:"..nr..",ret:"..(r or '').."\n")
    test_timeout2();
end
function test_timeout2()
    yaos.set_timeout(cb2,1000);
end

local function test_timeout()
  yaos.set_timeout(
    function () 
        nr = nr + 1
        yaos.say("time out in lua "..nr.."\n"); 
        test_timeout()
    end
    
,1000);
end
local function test_promise1()
    local t=yaos.set_timeout(1000)
    t:next(function(r)
        yaos.say("lua1:r:"..r.."\n")
        return r+1
    end):next(function(r)
        yaos.say("lua2:r:"..r.."\n");
        return r+2
    end):next(function(r)
        yaos.say("lua3:r:"..r.."\n");
        return r+3
    end)
end
local function test_promise2()
    local t=yaos.set_timeout(1000)
    local t1,t2,t3,t4
    t1=t:next(function (r)
        yaos.say("@@@@@@@lua timeout:"..r.."\n");

        t4= yaos.set_timeout(2000):next(function (r) return r+1000 end);
        yaos.say("t4:\n"..obj_dump(t4))
        return t4;
    end)
    t2=t1:next(function (r)
       yaos.say("##########lua timeout2:"..r.."\n");
       return r+100;

    end)
    yaos.say("t1:\n"..obj_dump(t1));
    yaos.say("t2:\n"..obj_dump(t2));

end
test_promise2();
--test_timeout();
--test_timeout2();
--[=[
local t=yaos.set_timeout(1000)
t:next(function(r)
    yaos.say("lua1:r:"..r.."\n")
    return r+1
end):next(function(r)
    yaos.say("lua2:r:"..r.."\n");
    return r+2
end):next(function(r)
    yaos.say("lua3:r:"..r.."\n");
    return r+3
end)
show_dump(t);
yaos.set_timeout(2000):next(function(r)
    return yaos.set_timeout(1000);
end):next(function(r)
    return yaos.set_timeout(1000);
end):next(function(r)
    return yaos.set_timeout(1000);
end):next(function(r)
    yaos.say("lua last get:"..r.."\n");
end)
yaos.set_timeout(8000):next(function(r)
    return yaos.set_timeout(1000);
end):next(function(r)
    return 123;
end):next(function(r)
    return yaos.set_timeout(1000):next(function(rr)
            return rr+r+1000
        end)
end):next(function(r)
    yaos.say("lua last get:"..r.."\n");
end)
]=]
_G['cb2']=cb2;
