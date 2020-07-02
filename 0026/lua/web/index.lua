yaos.say("web_index");
local html = [=[
<html>
<head><title>FROM LUA:lwIP - A Lightweight TCP/IP Stack</title></head>
<body bgcolor="white" text="black">

    <table width="100%">
      <tr valign="top"><td width="80">
          <a href="http://www.sics.se/"><img src="/img/sics.gif"
          border="0" alt="SICS logo" title="SICS logo"></a>
        </td><td width="500">
          <h1>lwIP - A Lightweight TCP/IP Stack</h1>
          <p>
            The web page you are watching was served by a simple web
            server running on top of the lightweight TCP/IP stack <a
            href="http://www.sics.se/~adam/lwip/">lwIP</a>.
          </p>
          <p>
            lwIP is an open source implementation of the TCP/IP
            protocol suite that was originally written by <a
            href="http://www.sics.se/~adam/lwip/">Adam Dunkels
            of the Swedish Institute of Computer Science</a> but now is
            being actively developed by a team of developers
            distributed world-wide. Since it's release, lwIP has
            spurred a lot of interest and has been ported to several
            platforms and operating systems. lwIP can be used either
            with or without an underlying OS.
          </p>
          <p>
            More information about lwIP can be found at the lwIP
            homepage at <a
            href="http://savannah.nongnu.org/projects/lwip/">http://savannah.nongnu.org/projects/lwip/</a>
            or at the lwIP wiki at <a
            href="http://lwip.wikia.com/">http://lwip.wikia.com/</a>.
          </p>
        </td><td>
          &nbsp;
        </td></tr>
      </table>
</body>
</html>

]=]
local headers = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8;\r\nContent-Length: "..#html.."\r\n\r\n"
ngx.say(headers)
ngx.say(html)
