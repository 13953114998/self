 
json = require("json")
uci = require("uci")

local function table_var_count( table_name )
	local count = 0;
	if table_name then
		for k,v in pairs(table_name) do
			count = count + 1;
		end
	end
	return count;
end
--[[=====================================================================================]]--
--[[ get freq status by ipaddr function ]]--
function get_base_band_status_freq( ip_in )
	local x = uci.cursor();
	local data = {};
	x:foreach("base_master", "base_band", function( s )
		local base_name = s[".name"];
		local ip = x:get("base_master", base_name, "ip");
		if ip == ip_in then
			data.ip = ip;
			data.sysuiarfcn = x:get("base_master", base_name, "sysuiarfcn");
			data.sysdiarfcn =  x:get("base_master", base_name, "sysdiarfcn");
			data.plmn = x:get("base_master", base_name, "plmn");
			data.sysbandwidth = x:get("base_master", base_name, "sysbandwidth");
			data.sysband = x:get("base_master", base_name, "sysband");
			data.pci = x:get("base_master", base_name, "pci");
			data.tac = x:get("base_master", base_name, "tac");
			data.cellid = x:get("base_master", base_name, "cellid");
			data.uepmax = x:get("base_master", base_name, "uepmax");
			data.enodebpmax = x:get("base_master", base_name, "enodebpmax");
		end
	end);
	return json.encode(data);
end

--[[ get txpower status by ipaddr function ]]--
local function get_base_band_status_txpower( ip_in )
	local x = uci.cursor();
	local txpower = 0;
	x:foreach("base_master", "base_band", function( s )
		local base_name = s[".name"];
		local ip = x:get("base_master", base_name, "ip");
		if ip == ip_in then
			txpower = x:get("base_master", base_name, "powerderease");
		end
	end);
	return txpower;
end

--[[ get rxgain status by ipaddr function ]]--
local function get_base_band_status_rxgain( ip_in )
	local x = uci.cursor();
	local rxgain = 0;
	x:foreach("base_master", "base_band", function( s )
		local base_name = s[".name"];
		local ip = x:get("base_master", base_name, "ip");
		if ip == ip_in then
			rxgain = x:get("base_master", base_name, "rxgain");
		end
	end);
	return rxgain;
end
--[[=====================================================================================]]--
--[[ compare freq config ,if diffrent ,return config json request , else return none ]]--
function compare_freq_config_status( json_str )
	local x = uci.cursor();
	local data = json.decode(json_str);
	local uci_data = {};
	x:foreach("base_master", "base_band", function ( s )
		local base_name = s[".name"];
		local ip = x:get("base_master", base_name, "ip");
		if ip == data.ip then
			uci_data.sysuiarfcn = x:get("base_master", base_name, "sysuiarfcn");
			uci_data.sysdiarfcn =  x:get("base_master", base_name, "sysdiarfcn");
			uci_data.plmn = x:get("base_master", base_name, "plmn");
			uci_data.sysbandwidth = x:get("base_master", base_name, "sysbandwidth");
			uci_data.sysband = x:get("base_master", base_name, "sysband");
			uci_data.pci = x:get("base_master", base_name, "pci");
			uci_data.tac = x:get("base_master", base_name, "tac");
			uci_data.cellid = x:get("base_master", base_name, "cellid");
			uci_data.uepmax = x:get("base_master", base_name, "uepmax");
			uci_data.enodebpmax = x:get("base_master", base_name, "enodebpmax");
		end
	end);
	if table_var_count(uci_data) == 0 then
		print("not found default config by uci ,IP:" .. data.ip);
		return "none";
	end
	if tostring(data.sysdiarfcn) ~= tostring(uci_data.sysdiarfcn) then
		uci_data.msgtype = "F003";
		uci_data.ip = data.ip;
		return json.encode(uci_data);
	elseif tostring(data.plmn) ~= tostring(uci_data.plmn) then
		uci_data.msgtype = "F003";
		uci_data.ip = data.ip;
		return json.encode(uci_data);
	elseif tostring(data.sysbandwidth) ~= tostring(uci_data.sysbandwidth) then
		uci_data.msgtype = "F003";
		uci_data.ip = data.ip;
		return json.encode(uci_data);
	elseif tostring(data.sysband) ~= tostring(uci_data.sysband) then
		uci_data.msgtype = "F003";
		uci_data.ip = data.ip;
		return json.encode(uci_data);
	elseif tostring(data.pci) ~= tostring(uci_data.pci) then
		uci_data.msgtype = "F003";
		uci_data.ip = data.ip;
		return json.encode(uci_data);
	elseif tostring(data.tac) ~= tostring(uci_data.tac) then
		uci_data.msgtype = "F003";
		uci_data.ip = data.ip;
		return json.encode(uci_data);
	elseif tostring(data.cellid) ~= tostring(uci_data.cellid) then
		uci_data.msgtype = "F003";
		uci_data.ip = data.ip;
		return json.encode(uci_data);
	elseif tostring(data.uepmax) ~= tostring(uci_data.uepmax) then
		uci_data.msgtype = "F003";
		uci_data.ip = data.ip;
		return json.encode(uci_data);
	elseif tostring(data.enodebpmax) ~= tostring(uci_data.enodebpmax) then
		uci_data.msgtype = "F003";
		uci_data.ip = data.ip;
		return json.encode(uci_data);
	else
		print("not found diffrent config option ..........");
		return "none";
	end
end

--[[compare sync config information, if diffrent ,return config json request , else return none ]]--
function compare_sync_config_status( json_str )
	local x = uci.cursor();
	local data = json.decode(json_str);
	local uci_data = {};
	x:foreach("base_master", "base_band", function ( s )
		local base_name = s[".name"];
		local ip = x:get("base_master", base_name, "ip");
		if ip == data.ip then
			uci_data.remmode = x:get("base_master", base_name, "sync_t");
		end
	end);
	if table_var_count(uci_data) == 0 then
		print("not found default config by uci ,IP:" .. data.ip);
		return "none";
	end
	if tostring(uci_data.remmode) ~= tostring(data.remmode) then
		uci_data.msgtype = "F023";
		uci_data.ip = data.ip;
		return json.encode(uci_data);
	end
	print("not found diffrent config option ..........");
	return "none";
end

--[[compare ue config information, if diffrent ,return config json request , else return none ]]--
function compare_ue_config_status( json_str )
	local x = uci.cursor();
	local data = json.decode(json_str);
	local uci_data = {};
	x:foreach("base_master", "base_band", function ( s )
		local base_name = s[".name"];
		local ip = x:get("base_master", base_name, "ip");
		if ip == data.ip then
			uci_data.work_mode = x:get("base_master", base_name, "ue_work_mode");
			uci_data.sub_mode = x:get("base_master", base_name, "ue_sub_mode");
			uci_data.capture_period = x:get("base_master", base_name, "ue_capture_period");
			uci_data.control_submode = x:get("base_master", base_name, "ue_control_submode");
		end
	end);
	if table_var_count(uci_data) == 0 then
		print("not found default config by uci ,IP:" .. data.ip);
		return "none";
	end
	if tostring(uci_data.work_mode) ~= tostring(data.work_mode) then
		uci_data.msgtype = "F006";
		uci_data.ip = data.ip;
		return json.encode(uci_data);
	elseif tostring(uci_data.sub_mode) ~= tostring(data.sub_mode) then
		uci_data.msgtype = "F006";
		uci_data.ip = data.ip;
		return json.encode(uci_data);
	elseif tostring(uci_data.capture_period) ~= tostring(data.capture_period) then
		uci_data.msgtype = "F006";
		uci_data.ip = data.ip;
		return json.encode(uci_data);
	elseif tostring(uci_data.control_submode) ~= tostring(data.control_submode) then
		uci_data.msgtype = "F006";
		uci_data.ip = data.ip;
		return json.encode(uci_data);
	end
	print("not found diffrent config option ..........");
	return "none";
end

--[[=====================================================================================]]--
--[[ get all base band device information by read /etc/config/base_master ]]--
function get_all_baseband_list(str)
	local x = uci.cursor();
	local count = 0;
	x:foreach("base_master", "base_band", function ( s )
		local base_name = s[".name"];
		local ip = x:get("base_master", base_name, "ip");
		local sysband = x:get("base_master", base_name, "sysband");
		local sysuiarfcn = x:get("base_master", base_name, "sysuiarfcn");
		local sysdiarfcn =  x:get("base_master", base_name, "sysdiarfcn");
		local plmn = x:get("base_master", base_name, "plmn");
		local sysbandwidth = x:get("base_master", base_name, "sysbandwidth");
		local sysband = x:get("base_master", base_name, "sysband");
		local pci = x:get("base_master", base_name, "pci");
		local tac = x:get("base_master", base_name, "tac");
		local cellid = x:get("base_master", base_name, "cellid");
		local uepmax = x:get("base_master", base_name, "uepmax");
		local enodebpmax = x:get("base_master", base_name, "enodebpmax");
		count = count + 1;
		if str == "simple" then
			print(count .. "\tip:" .. ip .. "\tBand:" .. sysband);
		elseif str == "freq" then
			print("base band dev[" .. count .. "]");
			print("\tip:" .. ip);
			print("\t" .. "sysuiarfcn:" .. sysuiarfcn);
			print("\t" .. "sysdiarfcn:" .. sysdiarfcn);
			print("\t" .. "plmn:" .. plmn);
			print("\t" .. "sysbandwidth:" .. sysbandwidth);
			print("\t" .. "sysband:" .. sysband);
			print("\t" .. "pci:" .. pci);
			print("\t" .. "tac:" .. tac);
			print("\t" .. "cellid:" .. cellid);
			print("\t" .. "uepmax:" .. uepmax);
			print("\t" .. "enodebpmax:" .. enodebpmax);
		end
	end);
end
--[[ get base band device information by read /etc/config/base_master by band ip addr ]]--
function get_freq_config_by_ip( ipaddr )
	local x = uci.cursor();
	local count = 0;
	local data = {};
	x:foreach("base_master", "base_band", function ( s )
		local base_name = s[".name"];
		local ip = x:get("base_master", base_name, "ip");
		if ip == ipaddr then
			data.sysband = tostring(x:get("base_master", base_name, "sysband"));
			data.sysuiarfcn = tostring(x:get("base_master", base_name, "sysuiarfcn"));
			data.sysdiarfcn =  tostring(x:get("base_master", base_name, "sysdiarfcn"));
			data.plmn = tostring(x:get("base_master", base_name, "plmn"));
			data.sysbandwidth = tostring(x:get("base_master", base_name, "sysbandwidth"));
			data.sysband = tostring(x:get("base_master", base_name, "sysband"));
			data.pci = tostring(x:get("base_master", base_name, "pci"));
			data.tac = tostring(x:get("base_master", base_name, "tac"));
			data.cellid = tostring(x:get("base_master", base_name, "cellid"));
			data.uepmax = tostring(x:get("base_master", base_name, "uepmax"));
			data.enodebpmax = tostring(x:get("base_master", base_name, "enodebpmax"));
		end
	end);
	if table_var_count(data) == 0 then
		return "none";
	end
	print("band(IP:" .. ipaddr .."), arfcn config info in '/etc/config/base_master':");
	print("      sysUlARFCN : " .. data.sysuiarfcn);
	print("      sysDlARFCN : " .. data.sysdiarfcn);
	print("            PLMN : " .. data.plmn);
	print("    sysBandwidth : " .. data.sysbandwidth);
	print("         sysBand : " .. data.sysband);
	print("             PCI : " .. data.pci);
	print("             TAC : " .. data.tac);
	print("          CellId : " .. data.cellid);
	print("          UePMax : " .. data.uepmax);
	print("      EnodeBPMax : " .. data.enodebpmax);
	data.ip = ipaddr;
	data.msgtype = "F003";
	return json.encode(data);
end

function get_wcdma_wm_config_by_ip( ipaddr )
	if ipaddr == nil or string.len(ipaddr) == 0 then
		return "none"
	end
	local x = uci.cursor();
	local data = {};
	x:foreach("base_master", "base_band", function ( s )
		local base_name = s[".name"];
		local ip = x:get("base_master", base_name, "ip");
		if ip == ipaddr then
			data.mcc = tostring(x:get("base_master", base_name, "mcc"));
			data.mnc = tostring(x:get("base_master", base_name, "mnc"));
			data.lac =  tostring(x:get("base_master", base_name, "lac"));
			data.rac = tostring(x:get("base_master", base_name, "rac"));
			data.psc = tostring(x:get("base_master", base_name, "psc"));
			data.arfcno = tostring(x:get("base_master", base_name, "arfcno"));
			data.cellid = tostring(x:get("base_master", base_name, "cellid"));
		end
	end);
	if table_var_count(data) == 0 then
		print("sorry, I can not found config information that ip is " .. ipaddr);
		return "none";
	end
	print("band(IP:" .. ipaddr .."), work_mode config info in '/etc/config/base_master':");
	print("      MCC : " .. data.mcc);
	print("      MNC : " .. data.mnc);
	print("      LAC : " .. data.lac);
	print("      RAC : " .. data.rac);
	print("      PSC : " .. data.psc);
	print("   Arfcno : " .. data.arfcno);
	print("   CellId : " .. data.cellid);
	data.ip = ipaddr;
	data.msgtype = "0xF08D";
	return json.encode(data);
end

function get_gsm_config_by_ip( num )
	if num == nil or string.len(num) == 0 then
		return "none"
	end
	local x = uci.cursor();
	local data = {};
	x:foreach("base_master", "base_band", function ( s )
		local base_name = s[".name"];
		local num_type = x:get("base_master", base_name, "type")
		if num_type == num then
			data.ip = tostring(x:get("base_master", base_name, "ip"));
			data.bcc = tostring(x:get("base_master", base_name, "bcc"));
			data.mcc = tostring(x:get("base_master", base_name, "mcc"));
			data.mnc = tostring(x:get("base_master", base_name, "mnc"));
			data.lac =  tostring(x:get("base_master", base_name, "lac"));
			data.lowatt = tostring(x:get("base_master", base_name, "lowatt"));
			data.upatt = tostring(x:get("base_master", base_name, "upatt"));
			data.cnum = tostring(x:get("base_master", base_name, "cnum"));
			data.cfgmode = tostring(x:get("base_master", base_name, "cfgmode"));
			data.workmode = tostring(x:get("base_master", base_name, "workmode"));
			data.startfreq_1 = tostring(x:get("base_master", base_name, "startfreq_1"));
			data.endfreq_1 = tostring(x:get("base_master", base_name, "endfreq_1"));
			data.startfreq_2 = tostring(x:get("base_master", base_name, "startfreq_2"));
			data.endfreq_2 = tostring(x:get("base_master", base_name, "endfreq_2"));
			data.freqoffset = tostring(x:get("base_master", base_name, "freqoffset"));
			data.msgtype= tostring(x:get("base_master", base_name, "msgtype"));
		end
	end);
	if table_var_count(data) == 0 then
		print("sorry, I can not found config information that ip is " .. data.ip);
		return "none";
	end
	print("band(IP:" .. data.ip .."), arfcn config info in '/etc/config/base_master':");
	print("              arfcn : " .. data.bcc);
	print("                MCC : " .. data.mcc);
	print("                MNC : " .. data.mnc);
	print("                LAC : " .. data.lac);
	print("            low ATT : " .. data.lowatt);
	print("             up ATT : " .. data.upatt);
	print("            cell ID : " .. data.cnum);
	print("        config mode : " .. data.cfgmode .. "    (0: 自动模式 1: 手动模式)");
	print("          work mode : " .. data.workmode .. "    (0: 2g驻留模式 1: 2g非驻留模式)");
	print("   900M start arfcn : " .. data.startfreq_1);
	print("     900M end arfcn : " .. data.endfreq_1);
	print("  1800M start arfcn : " .. data.startfreq_2);
	print("    1800M end arfcn : " .. data.endfreq_2);
	print("       arfcn offset : " .. data.freqoffset);
	if tonumber(num_type) == 1 then
		data.msgtype = "A003";
	else
		data.msgtype = "A004";
	end
	return json.encode(data);
end

--[[ pares WCDMA base_band device information json string ]]--
function pares_wcdma_base_band_json_information( json_str )
	if json_str == nil or string.len(json_str) == 0 then
		return;
	end
	if json_str == "nothing" then
		print("not get information!");
		return;
	end
	-- print(json_str);
	local data = json.decode(json_str);
	print("WCDMA information:");
	for k, v in pairs(data) do
		if k == "ip" or k == "software_version" then
			print(string.format("%20s", k) .. " : " .. v);
		end
	end
	print("status and arcfn information");
	for k, v in pairs(data) do
		if k ~= "ip" and k ~= "software_version" and k ~= "topic" then
			if k == "work_mode" then
				print(string.format("%20s", k) .. " : WCDMA");
			elseif k == "online" then
				if v * 1 == 1 then
					print(string.format("%20s", k) .. " : Yes!");
				else
					print(string.format("%20s", k) .. " : No!");
				end
			elseif k == "pa_power" then
				local key_tmp = "power of PA";
				print(string.format("%20s", key_tmp) .. " : " .. v .. " W");
			elseif k == "mode" then
				local key_tmp = "drive mode";
				if v * 1 == 0 then
					print(string.format("%20s", key_tmp) .. " : drive to GSM");
				elseif v * 1 == 1 then
					print(string.format("%20s", key_tmp) .. " : NAS REJECT");
				end
			elseif k == "fre_ratio" then
				local key_tmp = "Pilot ratio";
				print(string.format("%20s", key_tmp) .. " : " .. v);
			else
				print(string.format("%20s", k) .. " : " .. v);
			end
		end
	end
end
--[[ pares LTE base_band device information json string ]]--
function pares_base_band_json_information( json_str )
	if json_str == nil or string.len(json_str) == 0 then
		return;
	end
	if json_str == "nothing" then
		print("not get information!");
		return;
	end
	local simple_data = {};
	local data = json.decode(json_str);
	print("LTE base band device list :");
	for num, value in pairs(data.stations) do
		table.insert(simple_data, num, value.ip);
		print("----------------------------------------");
		if value.work_mode * 1  <= 1 then
			print("LTE Base Band device No." .. num);
			local status_state;
			local freq_state;
			print("generic information")
			for k, v in pairs(value) do
				if k == "work_mode" then
					if v * 1 == 0 then
						print(string.format("%20s", k) .. " : TDD (LTE)");
					elseif v * 1 == 1 then
						print(string.format("%20s", k) .. " : FDD (LTE)");
					end
				elseif k == "online" then
					if v * 1 == 1 then
						print(string.format("%20s", k) .. " : Yes!");
					else
						print(string.format("%20s", k) .. " : No!");
					end
				elseif k == "freq_conf" then
					freq_state = v;
				elseif k == "state" then
					status_state = v;
				elseif k == "pa_num" then
					local key_tmp = "PA num";
					print(string.format("%20s", key_tmp) .. " : " .. v);
				else
					if k ~= "device_model" and k ~= "hardware_version" then
						print(string.format("%20s", k) .. " : " .. v);
					end
				end
			end
			print("status information");
			for k, v in pairs(status_state) do
				if k == "sync_type" then
					local key_tmp = "sync mode";
					if v * 1 == 0 then
						print(string.format("%20s", key_tmp) .. " : Air interface " );
					elseif v * 1 == 1 then
						print(string.format("%20s", key_tmp) .. " : GPS" );
					else
						print(string.format("%20s", key_tmp) .. " : Unknow" );
					end
				elseif k == "sync_state" then
					local key_tmp = "sync status";
					if v * 1 == 0 then
						print(string.format("%20s", key_tmp) .. " : GPS sync success!");
					elseif v == 1 then
						print(string.format("%20s", key_tmp) .. " : Air interface sync success!");
					else
						print(string.format("%20s", key_tmp) .. " : Sync failed!");
					end
				elseif k == "power_derease" then
					local key_tmp = "TX-power";
					print(string.format("%20s", key_tmp) .. " : " .. v);
				elseif k == "regain" then
					local key_tmp = "RX-gain";
					print(string.format("%20s", key_tmp) .. " : " .. v);
				elseif k == "cell_state" then
					local key_tmp = "cell status";
					if v * 1 == 0 then
						print(string.format("%20s", key_tmp) .. " : Cell is Idle !");
					elseif v * 1 == 1 then
						print(string.format("%20s", key_tmp) .. " : Cell is REM executing!");
					elseif v * 1 == 2 then
						print(string.format("%20s", key_tmp) .. " : Cell is activating!");
					elseif v * 1 == 3 then
						print(string.format("%20s", key_tmp) .. " : Cell is activation!");
					elseif v * 1 == 4 then
						print(string.format("%20s", key_tmp) .. " : Cell is De activating!");
					else
						print(string.format("%20s", key_tmp) .. " : Unknow!");
					end
				else
					print(string.format("%20s", k) .. " : " .. v);
				end
			end
			print("freq information");
			for k, v in pairs(freq_state) do
				print(string.format("%20s", k) .. " : " .. v);
			end
		end
		print("----------------------------------------");
	end
	print("");
	print("--------------------------------------------");
	print("There have " .. table_var_count(simple_data) .. " LTE base band device.");
	for k, v in pairs(simple_data) do
		print(" device No." .. k .. ", IP: " .. v);
	end
	print("please config with ipaddr!");
	print("--------------------------------------------");
end

--[[ pares GSM base_band device information json string ]]--
function pares_gsm_base_band_json_information( json_str )
	print(json_str);
	local data = json.decode(json_str);
	print("GSM information:");
	for k, v in pairs(data) do
		if k == "ip" or k == "software_version" then
			print(string.format("%20s", k) .. " : " .. v);
		end
	end
	print("status and arcfn information");
	for k, v in pairs(data) do
		if k ~= "ip" and k ~= "software_version" and k ~= "topic" then
			if k == "online" then
				if v * 1 == 1 then
					print(string.format("%20s", k) .. " : Yes!");
				else
					print(string.format("%20s", k) .. " : No!");
				end
			elseif k == "bcc" then
				local key_tmp = "arfcn";
				print(string.format("%20s", key_tmp) .. " : " .. v);
			elseif k == "mcc" then
				local key_tmp = "MCC";
				print(string.format("%20s", key_tmp) .. " : " .. v);
			elseif k == "mnc" then
				local key_tmp = "MNC";
				print(string.format("%20s", key_tmp) .. " : " .. v);
			elseif k == "lac" then
				local key_tmp = "LAC";
				print(string.format("%20s", key_tmp) .. " : " .. v);
			elseif k == "lowatt" then
				local key_tmp = "low ATT";
				print(string.format("%20s", key_tmp) .. " : " .. v);
			elseif k == "upatt" then
				local key_tmp = "up ATT";
				print(string.format("%20s", key_tmp) .. " : " .. v);
			elseif k == "cnum" then
				local key_tmp = "cell num";
				print(string.format("%20s", key_tmp) .. " : " .. v);
			elseif k == "cfgmode" then
				local key_tmp = "config mode";
				if v * 1 == 1 then
					print(string.format("%20s", key_tmp) .. " : 手动模式");
				else
					print(string.format("%20s", key_tmp) .. " : 自动模式");
				end
			elseif k == "workmode" then
				local key_tmp = "work mode";
				if v * 1 == 2 then
					print(string.format("%20s", key_tmp) .. " : 2g驻留模式");
				else
					print(string.format("%20s", key_tmp) .. " : 2g非驻留模式");
				end
			elseif k == "startfreq_1" then
				local key_tmp = "900M start arfcn";
				print(string.format("%20s", key_tmp) .. " : " .. v);
			elseif k == "endfreq_1" then
				local key_tmp = "900M end arfcn";
				print(string.format("%20s", key_tmp) .. " : " .. v);
			elseif k == "startfreq_2" then
				local key_tmp = "1800M start arfcn";
				print(string.format("%20s", key_tmp) .. " : " .. v);
			elseif k == "endfreq_2" then
				local key_tmp = "1800M end arfcn";
				print(string.format("%20s", key_tmp) .. " : " .. v);
			elseif k == "freqoffset" then
				local key_tmp = "arfcn offset";
				print(string.format("%20s", key_tmp) .. " : " .. v);
			else
				print(string.format("%20s", k) .. " : " .. v);
			end
		end
	end
end


--[[
local json_str = get_base_band_status_txpower("192.168.2.37");
print(json_str);
json_str = get_base_band_status_rxgain("192.168.2.38");
print(json_str);
json_str = get_base_band_status_freq("192.168.2.36");
print(json_str);
]]--

--[[
local sttr = "{\"topic\":\"sensor.00-0C-29-A2-23-52\",\"ip\":\"192.168.2.53\",\"sysuiarfcn\":37900,\"sysdiarfcn\":37900,\"plmn\":\"46000\",\"sysbandwidth\":100,\"sysband\":38,\"pci\":111,\"tac\":1274,\"cellid\":123,\"uepmax\":21,\"enodebpmax\":20}"
local return_s = compare_freq_config_status(sttr);
print("after compare, return string is: " .. return_s);
]]

--local str = "{\"topic\":\"sensor.00-0C-29-A2-23-52\",\"ip\":\"192.168.2.53\",\"remmode\":0,\"sync_state\":2}"
--local xxx = compare_sync_config_status(str);
--print("----------------xxx:" .. xxx);


-- local str = "{\"topic\":\"sensor.00-0C-29-A2-23-52\",\"ip\":\"192.168.2.53\",\"sysuiarfcn\":37900,\"sysdiarfcn\":37900,\"plmn\":\"46000\",\"sysbandwidth\":100,\"sysband\":38,\"pci\":111,\"tac\":1274,\"cellid\":123,\"uepmax\":21,\"enodebpmax\":20}"
-- pares_base_band_json_information(str);
--local str = "{\"topic\":\"sensor.00-0C-29-A2-23-52\",\"stations\":[{\"ip\":\"192.168.178.202\",\"online\":\"1\",\"work_mode\":3,\"software_version\":\"1.2.2\",\"hardware_version\":\"N/A\",\"device_model\":\"N/A\",\"pa_num\":\"N/A\",\"mac\":\"00:15:E1:03:21:27\",\"sn\":\"N/A\"}]}"
--pares_gsm_base_band_json_information(str);
