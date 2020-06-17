yaos.say("33333333Hello lua\n");
local nr = 0;
local function cb3(r)
    nr = nr + 1;
    yaos.say("test3.lua,NR:"..nr..",ret:"..r);
end
_G['cb3']=cb3;
