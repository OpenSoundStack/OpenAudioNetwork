osst_hdr = Proto("osstcomhdr", "OSST Header")

hdr_ptype = ProtoField.uint8("osstcomhdr.ptype", "packetType", base.HEX)
hdr_version = ProtoField.uint16("osstcomhdr.ver", "version", base.HEX)
hdr_flags = ProtoField.uint16("osstcomhdr.flags", "flags", base.HEX)
hdr_tstamp = ProtoField.uint64("osstcomhdr.tstamp", "timestamp", base.DEC)
hdr_prev_delay = ProtoField.uint64("osstcomhdr.pdelay", "previousDelay", base.DEC)

osst_hdr.fields = {
	hdr_ptype,
	hdr_version,
	hdr_flags,
	hdr_tstamp,
	hdr_prev_delay
}

function osst_hdr.dissector(buffer, pinfo, tree)
	local length = buffer:len()
	if length == 0 then
		return
	end

	pinfo.cols.protocol = "OAN Audio"

	local ptype_number = buffer(0, 4):le_uint()
	local ptype_name = get_oan_packet_type(ptype_number)

	local subtree = tree:add(osst_hdr, buffer(), "OSST Common Header")
	subtree:add_le(hdr_ptype,      buffer(0,  4)):append_text(" (" .. ptype_name .. ") ")
	subtree:add_le(hdr_version,    buffer(4,  2))
	subtree:add_le(hdr_flags,      buffer(6,  2))
	subtree:add_le(hdr_tstamp,     buffer(8,  8))
	subtree:add_le(hdr_prev_delay, buffer(16, 8))

	local com_hdr_len = 24
	local new_buffer = buffer(com_hdr_len):tvb()
	local next_dissector = Dissector.get("data")

	if ptype_number == 0x00 and Dissector.get("osstmapping") ~= nil then
		next_dissector = Dissector.get("osstmapping")
	elseif ptype_number == 0x02 and Dissector.get("osstctrlpnew") then
		next_dissector = Dissector.get("osstctrlpnew")
	elseif ptype_number == 0x03 and Dissector.get("osstctrlresp") then
		next_dissector = Dissector.get("osstctrlresp")
	end

	next_dissector:call(new_buffer, pinfo, subtree)
end

function get_oan_packet_type(packet_type_hex)
	local pname = "Unknown"

	if packet_type_hex == 0x00 then pname = "Mapping"
	elseif packet_type_hex == 0x01 then pname = "Control"
	elseif packet_type_hex == 0x02 then pname = "Control Create"
	elseif packet_type_hex == 0x03 then pname = "Control Response"
	elseif packet_type_hex == 0x04 then pname = "Control Query"
	elseif packet_type_hex == 0x05 then pname = "Audio"
	elseif packet_type_hex == 0x06 then pname = "Clock Sync"
	end

	return pname
end