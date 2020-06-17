yaos.say("11111111111111hello lua\n");
local nr = 0;
local function cb1(r)
    nr = nr + 1;
    yaos.say("test1.lua,NR:"..nr..",ret:"..r);
end
_G['cb1']=cb1;
