function trim(s)
  return (s:gsub("^%s*(.-)%s*$", "%1"))
end

local authstatus = trim(luci.sys.exec("uci get system.@system[0].status"))
local machinecode = luci.sys.exec("uci get system.@system[0].id")

local m, s, p, o

m = Map("system")

if authstatus == "1" then
	s = m:section(TypedSection, "system", translate("授权状态:已授权"))
	s.addremove = false
	s.anonymous = true               
	luci.sys.exec("uci set system.@system[0].authword='已授权'")
	luci.sys.exec("uci commit system")
	s:option(DummyValue,"id","机器码");
else
	s = m:section(TypedSection, "system", translate("授权状态:未授权"))
	s.addremove = false
	s.anonymous = true               
	luci.sys.exec("uci set system.@system[0].authword='未授权'")
	luci.sys.exec("uci commit system")
	s:option(DummyValue,"id","机器码");
	s:option(Value, "auth", translate("授权码"))
end

local apply = luci.http.formvalue("cbi.apply")
if apply then
	io.popen("killall -9 base_master")
	io.popen("base_master &")
end

return m 
