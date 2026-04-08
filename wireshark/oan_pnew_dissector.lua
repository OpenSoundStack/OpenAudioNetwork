oan_ctrl_pipenew = Proto("osstctrlpnew", "OAN Create Pipe")

oan_pnew_channel = ProtoField.uint8("oan.ctrl.pipenew.channel", "channel", base.DEC)
oan_pnew_stack_pos = ProtoField.uint8("oan.ctrl.pipenew.stack_pos", "stackPos", base.DEC)
oan_pnew_seq = ProtoField.uint8("oan.ctrl.pipenew.seq", "seq", base.DEC)
oan_pnew_seq_max = ProtoField.uint8("oan.ctrl.pipenew.seq_max", "seqMax", base.DEC)
oan_pnew_elem_type = ProtoField.string("oan.ctrl.pipenew.elem_type", "elemType")

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

