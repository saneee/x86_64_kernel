local ctx = ngx.ctx
local concat = table.concat
local req = ctx.req
local html = "query body:\n"..concat(req and req.body_arr or {})
local headers = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8;\r\nContent-Length: "..#html.."\r\n\r\n"
ngx.say(headers)
ngx.say(html)

