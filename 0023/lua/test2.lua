yaos.say("22222222222,test2.lua\n");
local nr = 0;
local function cb2(r)
    nr = nr + 1;
    yaos.say("test2.lua,NR:"..nr..",ret:"..r);
end
--yaos.set_timeout(cb2,1000);
yaos.set_timeout(function () yaos.say("time out in lua\n"); end, 1000);
_G['cb2']=cb2;
