oan_mapping = Proto("osstmapping", "OAN Mapping")

oan_map_devname = ProtoField.string("oan.map.dname", "deviceName", base.ASCII)
oan_map_self_addr = ProtoField.ether("oan.map.saddr", "selfAddress")
oan_map_self_uid = ProtoField.uint16("oan.map.suid", "selfUid", base.DEC)
oan_map_device_type = ProtoField.uint16("oan.map.dtype", "deviceType", base.HEX)
oan_map_sampling_rate = ProtoField.uint32("oan.map.srate", "sampleRate", base.HEX)
oan_map_node_topo = ProtoField.none("oan.map.node_topo", "nodeTopo")
oan_map_cktype = ProtoField.uint32("oan.map.cktype", "clockType", base.HEX)

oan_mapping.fields = {
	oan_map_devname,
	oan_map_self_addr,
	oan_map_self_uid,
	oan_map_device_type,
	oan_map_sampling_rate,
	oan_map_node_topo,
	oan_map_cktype
}

function oan_mapping.dissector(buffer, pinfo, tree)
	local length = buffer:len()
	if length == 0 then
		return
	end

	pinfo.cols.info:append("OAN Mapping Packet")

	local dtype = get_oan_device_type_name(buffer(42, 2):le_uint())
	local ck_type = get_oan_clock_type_name(buffer(48+20, 4):le_uint())

	local subtree = tree:add(oan_mapping, buffer(), "OAN Mapping Packet")
	subtree:add_le(oan_map_devname, buffer(0, 32))
	subtree:add_le(oan_map_self_addr, buffer(32, 6))
	subtree:add_le(oan_map_self_uid, buffer(40, 2)) -- Addr encoded as uint64_t, hence the 2 bytes offset
	subtree:add_le(oan_map_device_type, buffer(42, 2)):append_text(" (" .. dtype .. ") ")
	subtree:add_le(oan_map_sampling_rate, buffer(44, 4))
	subtree:add_le(oan_map_node_topo, buffer(48, 20))
	subtree:add_le(oan_map_cktype, buffer(48+20, 4)):append_text(" (" .. ck_type .. ") ")
end

function get_oan_clock_type_name(clock_type_hex)
	local pname = "Unknown"

	if clock_type_hex == 0x00 then pname = "None"
	elseif clock_type_hex == 0x01 then pname = "Master"
	elseif clock_type_hex == 0x02 then pname = "Slave"
	end

	return pname
end

function get_oan_device_type_name(device_type_hex)
	local pname = "Unknown"

	if device_type_hex == 0x00 then pname = "Control Surface"
	elseif device_type_hex == 0x01 then pname = "Monitoring"
	elseif device_type_hex == 0x02 then pname = "Audio IO Interface"
	elseif device_type_hex == 0x03 then pname = "Audio DSP"
	end

	return pname
end
