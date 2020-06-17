function add2(x,y)
      error("hi");
      return x + y * 100

end
function add(x,y)
   if not pcall(function() return add2(x,y); end) then
     return 0;
   end
end
