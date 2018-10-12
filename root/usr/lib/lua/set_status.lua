
json = require("json")
uci = require("uci")
--[[ 检查table种所含的元素个数 ]]
local function table_var_count( table_name )
	local count = 0;
	if type(table_name) ~= "table" then
		return count;
	end
	if table_name then
		for k,v in pairs(table_name) do
			count = count + 1;
		end
	end
	return count;
end


--[[ 设置默认频点信息 ]]--
function set_base_band_arfcn_defconfig( json_str )
	if json_str == nil then
		return
	end
	print("---------------" .. json_str);
	local data = json.decode(json_str);
	local x = uci.cursor();
	x:foreach("base_master", "base_band", function ( s )
		local base_name = s[".name"];
		local ip = x:get("base_master", base_name, "ip");
		if ip == data.ip then
			if ip == "192.168.2.36" then
				if data.stype * 1 == 1 then --yi pin
					x:set("base_master", base_name, "sysuiarfcn", "19925");
					x:set("base_master", base_name, "sysdiarfcn", "1925");
					x:set("base_master", base_name, "plmn","46001");
					x:set("base_master", base_name, "sysbandwidth", "25");
					x:set("base_master", base_name, "sysband", "3");
					x:set("base_master", base_name, "pci", "501");
					x:set("base_master", base_name, "tac", "12345");
					x:set("base_master", base_name, "cellid", "123");
					x:set("base_master", base_name, "uepmax", "23");
					x:set("base_master", base_name, "enodebpmax", "20");
				else -- tong pin
					x:set("base_master", base_name, "sysuiarfcn", "19650");
					x:set("base_master", base_name, "sysdiarfcn", "1650");
					x:set("base_master", base_name, "plmn", "46001");
					x:set("base_master", base_name, "sysbandwidth", "25");
					x:set("base_master", base_name, "sysband", "3");
					x:set("base_master", base_name, "pci", "501");
					x:set("base_master", base_name, "tac", "12345");
					x:set("base_master", base_name, "cellid", "123");
					x:set("base_master", base_name, "uepmax", "23");
					x:set("base_master", base_name, "enodebpmax", "20");
				end
			elseif ip == "192.168.2.38" then
				if data.stype * 1 == 1 then -- yi pin
					x:set("base_master", base_name, "sysuiarfcn", "255");
					x:set("base_master", base_name, "sysdiarfcn", "39325");
					x:set("base_master", base_name, "plmn", "46000");
					x:set("base_master", base_name, "sysbandwidth", "25");
					x:set("base_master", base_name, "sysband", "40");
					x:set("base_master", base_name, "pci", "501");
					x:set("base_master", base_name, "tac", "12345");
					x:set("base_master", base_name, "cellid", "123");
					x:set("base_master", base_name, "uepmax", "23");
					x:set("base_master", base_name, "enodebpmax", "20");
				else -- tong pin
					x:set("base_master", base_name, "sysuiarfcn", "255");
					x:set("base_master", base_name, "sysdiarfcn", "37900");
					x:set("base_master", base_name, "plmn", "46000");
					x:set("base_master", base_name, "sysbandwidth", "25");
					x:set("base_master", base_name, "sysband", "38");
					x:set("base_master", base_name, "pci", "501");
					x:set("base_master", base_name, "tac", "1264");
					x:set("base_master", base_name, "cellid", "123");
					x:set("base_master", base_name, "uepmax", "23");
					x:set("base_master", base_name, "enodebpmax", "20");
				end
			end
		end
	end);
	x:commit("base_master");
	return;
end

--[[ 设置频点信息 ]]--
function set_base_band_status_freq( json_str )
	local x = uci.cursor();
	local data = json.decode(json_str);
	if table_var_count(data) == 0 then
		return;
	end
	for k, v in pairs(data) do
		print("k:" .. k .. " v:" .. v);
	end
	x:foreach("base_master", "base_band", function ( s )
		local base_name = s[".name"];
		local ip = x:get("base_master", base_name, "ip");
		if ip == data.ip then
			print("base_name" .. base_name .. "ip:" .. ip);
			x:set("base_master", base_name, "sysuiarfcn", data.sysuiarfcn);
			x:set("base_master", base_name, "sysdiarfcn", data.sysdiarfcn);
			x:set("base_master", base_name, "plmn", data.plmn);
			x:set("base_master", base_name, "sysbandwidth", data.sysbandwidth);
			x:set("base_master", base_name, "sysband", data.sysband);
			x:set("base_master", base_name, "pci", data.pci);
			x:set("base_master", base_name, "tac", data.tac);
			x:set("base_master", base_name, "cellid", data.cellid);
			x:set("base_master", base_name, "uepmax", data.uepmax);
			x:set("base_master", base_name, "enodebpmax", data.enodebpmax);
		end
	end);
	x:commit("base_master");
	return;
end
function set_base_band_status_txrxpower( json_str )
	local x = uci.cursor();
	local data = json.decode(json_str);
	if table_var_count(data) == 0 then
		return;
	end
	for k, v in pairs(data) do
		print("k:" .. k .. " v:" .. v);
	end
	x:foreach("base_master", "base_band", function ( s )
		local base_name = s[".name"];
		local ip = x:get("base_master", base_name, "ip");
		if ip == data.ip then
			end
	end);
	x:commit("base_master");
	return;
end
--[[ 设置同步方式 ]]--
function set_base_band_status_sync( json_str )
	local x = uci.cursor();
	local data = json.decode(json_str);
	if table_var_count(data) == 0 then
		return;
	end
	x:foreach("base_master", "base_band", function ( s )
		local base_name = s[".name"];
		local ip = x:get("base_master", base_name, "ip");
		if ip == data.ip then
			x:set("base_master", base_name, "sync_t", data.remmode);
		end
	end);
	x:commit("base_master");
end
--[[ 设置辅PLMN信息 ]]--
function set_base_band_status_plmn_list( json_str )
	local data = json.decode(json_str);
	if table_var_count(data) == 0 then
		return;
	end
	local set_list = {};
	for m, n in pairs(data.plmn_list) do
		table.insert(set_list, n);
	end

	x:foreach("base_master", "base_band", function ( s )
		local base_name = s[".name"];
		local ip = x:get("base_master", base_name, "ip");
		if ip == data.ip then
			x:set("base_master", base_name, "plmnlist", set_list);
		end
	end);
	x:commit("base_master");
end
--[[
local str="{\"msgtype\":\"0xF060\",\"ip\":\"192.168.2.53\",\"plmn_num\":\"2\",\"plmn_list\":[\"46000\",\"46011\"]}"
set_base_band_status_plmn_list(str);
]]
--[[
local str ="{\"ip\":\"192.168.2.36\",\"remmode\":\"0\"}";
set_base_band_status_sync(str);
]]


--local str = "{\"ip\":\"192.168.2.36\", \"stype\":\"1\"}";
--set_base_band_arfcn_defconfig(str);
