osst_llhdr = Proto("osstllp", "OSST Low Latency Packet")

llhdr_sender_uid = ProtoField.uint16("osstllhdr.sender_uid", "senderUid", base.DEC)
llhdr_dest_uid = ProtoField.uint16("osstllhdr.dest_uid", "destUid", base.DEC)
llhdr_packet_size = ProtoField.uint16("osstllhdr.packet_size", "packetSize", base.DEC)

osst_llhdr.fields = {
    llhdr_sender_uid,
    llhdr_dest_uid,
    llhdr_packet_size
}

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
    pinfo.cols.info = llhdr_sender .. " -> " .. llhdr_recv .. ", "

    local subtree = tree:add(osst_llhdr, buffer(), "OSST Low Lat Packet")
    subtree:add_le(llhdr_sender_uid, buffer(0, 2))
    subtree:add_le(llhdr_dest_uid, buffer(2, 2))
    subtree:add_le(llhdr_packet_size, buffer(4, 2))

    local hdr_len = 6
    local tvb = buffer(hdr_len):tvb()

    Dissector.get("osstcomhdr"):call(tvb, pinfo, subtree)
end

local eth_proto = DissectorTable.get("ethertype")
eth_proto:add(0x681, osst_llhdr)
eth_proto:add(0x682, osst_llhdr)
eth_proto:add(0x683, osst_llhdr)
eth_proto:add(0x684, osst_llhdr)
