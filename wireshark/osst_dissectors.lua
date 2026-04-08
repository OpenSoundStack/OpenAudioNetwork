osst_llhdr = Proto("osstllp", "OSST Low Latency Packet")
osst_hdr = Proto("osstcomhdr", "OSST Header")
oan_mapping = Proto("osstmapping", "OAN Mapping")
oan_ctrl_pipenew = Proto("osstctrlpnew", "OAN Create Pipe")

hdr_ptype = ProtoField.uint8("osstcomhdr.ptype", "packetType", base.HEX)
hdr_version = ProtoField.uint16("osstcomhdr.ver", "version", base.HEX)
hdr_flags = ProtoField.uint16("osstcomhdr.flags", "flags", base.HEX)
hdr_tstamp = ProtoField.uint64("osstcomhdr.tstamp", "timestamp", base.DEC)
hdr_prev_delay = ProtoField.uint64("osstcomhdr.pdelay", "previousDelay", base.DEC)

llhdr_sender_uid = ProtoField.uint16("osstllhdr.sender_uid", "senderUid", base.DEC)
llhdr_dest_uid = ProtoField.uint16("osstllhdr.dest_uid", "destUid", base.DEC)
llhdr_packet_size = ProtoField.uint16("osstllhdr.packet_size", "packetSize", base.DEC)

oan_map_devname = ProtoField.string("oan.map.dname", "deviceName", base.ASCII)
oan_map_self_addr = ProtoField.ether("oan.map.saddr", "selfAddress")
oan_map_self_uid = ProtoField.uint16("oan.map.suid", "selfUid", base.DEC)
oan_map_device_type = ProtoField.uint16("oan.map.dtype", "deviceType", base.HEX)
oan_map_sampling_rate = ProtoField.uint32("oan.map.srate", "sampleRate", base.HEX)
oan_map_node_topo = ProtoField.none("oan.map.node_topo", "nodeTopo")
oan_map_cktype = ProtoField.uint32("oan.map.cktype", "clockType", base.HEX)

oan_pnew_channel = ProtoField.uint8("oan.ctrl.pipenew.channel", "channel", base.DEC)
oan_pnew_stack_pos = ProtoField.uint8("oan.ctrl.pipenew.stack_pos", "stackPos", base.DEC)
oan_pnew_seq = ProtoField.uint8("oan.ctrl.pipenew.seq", "seq", base.DEC)
oan_pnew_seq_max = ProtoField.uint8("oan.ctrl.pipenew.seq_max", "seqMax", base.DEC)
oan_pnew_elem_type = ProtoField.string("oan.ctrl.pipenew.elem_type", "elemType")

osst_llhdr.fields = {
    llhdr_sender_uid,
    llhdr_dest_uid,
    llhdr_packet_size
}

osst_hdr.fields = {
    hdr_ptype,
    hdr_version,
    hdr_flags,
    hdr_tstamp,
    hdr_prev_delay
}

oan_mapping.fields = {
    oan_map_devname,
    oan_map_self_addr,
    oan_map_self_uid,
    oan_map_device_type,
    oan_map_sampling_rate,
    oan_map_node_topo,
    oan_map_cktype
}

oan_ctrl_pipenew.fields = {
    oan_pnew_channel,
    oan_pnew_stack_pos,
    oan_pnew_seq,
    oan_pnew_seq_max,
    oan_pnew_elem_type
}

function oan_ctrl_pipenew.dissector(buffer, pinfo, tree)
    local length = buffer:len()
    if length == 0 then
        return
    end

    local seq = buffer(2, 1):le_uint() + 1
    local seq_max = buffer(3, 1):le_uint()

    pinfo.cols.info = "OAN Control Pipe Create Packet, Seq = " .. seq .. "/" .. seq_max

    local subtree = tree:add(oan_ctrl_pipenew, buffer(), "OAN Control Pipe Create")

    subtree:add_le(oan_pnew_channel,    buffer(0, 1))
    subtree:add_le(oan_pnew_stack_pos,  buffer(1, 1))
    subtree:add_le(oan_pnew_seq,        buffer(2, 1))
    subtree:add_le(oan_pnew_seq_max,    buffer(3, 1))
    subtree:add_le(oan_pnew_elem_type,  buffer(4, 32))
end

function oan_mapping.dissector(buffer, pinfo, tree)
    local length = buffer:len()
    if length == 0 then
        return
    end

    pinfo.cols.info = "OAN Mapping Packet"

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
function osst_hdr.dissector(buffer, pinfo, tree)
    local length = buffer:len()
    if length == 0 then
        return
    end

    pinfo.cols.protocol = "OAN Audio"
    pinfo.cols.info = "OAN Unknown Packet"

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
    if ptype_number == 0x00 then
        oan_mapping.dissector(new_buffer, pinfo, subtree)
    elseif ptype_number == 0x02 then
        oan_ctrl_pipenew.dissector(new_buffer, pinfo, subtree)
    else
        Dissector.get("data"):call(new_buffer, pinfo, subtree)
    end
end

function osst_llhdr.dissector(buffer, pinfo, tree)
    local length = buffer:len()
    if length == 0 then
        return
    end

    local llhdr_sender = buffer(0, 2):le_uint()

    local llhdr_recv = buffer(2, 2):le_uint()
    if llhdr_recv == 0 then
        llhdr_recv = "Broadcast"
    end

    pinfo.cols.protocol = "OSST Low Lat Packet"
    pinfo.cols.info = llhdr_sender .. " -> " .. llhdr_recv

    local subtree = tree:add(osst_llhdr, buffer(), "OSST Low Lat Packet")
    subtree:add_le(llhdr_sender_uid, buffer(0, 2))
    subtree:add_le(llhdr_dest_uid, buffer(2, 2))
    subtree:add_le(llhdr_packet_size, buffer(4, 2))

    local hdr_len = 6
    local tvb = buffer(hdr_len):tvb()

    osst_hdr.dissector(tvb, pinfo, subtree)
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

local eth_proto = DissectorTable.get("ethertype")
eth_proto:add(0x681, osst_llhdr)
eth_proto:add(0x682, osst_llhdr)
eth_proto:add(0x683, osst_llhdr)
eth_proto:add(0x684, osst_llhdr)
