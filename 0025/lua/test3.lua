yaos.say("33333333Hello lua\n");
function callback()
yaos.say("callback\n")
end
function defcallback()
yaos.say("predef callback\n")
end
yaos.setnotify(1000,callback)
yaos.testnotify()
yaos.testenv()
