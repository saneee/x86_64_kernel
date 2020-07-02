local cjson=require("cjson")
local ctx = ngx.ctx
local concat = table.concat
local req = ctx.req
local cut = string.sub
local body = cjson.encode(req)
local html = body
local headers = "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=UTF-8;\r\nContent-Length: "..#html.."\r\n\r\n"
ngx.say(headers)
ngx.say(html)

