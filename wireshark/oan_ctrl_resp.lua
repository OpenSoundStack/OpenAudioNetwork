local resp_codes_valstr = {
	[1]  = "CREATE_OK",           -- 1 << 0
	[2]  = "CREATE_ERROR",        -- 1 << 1
	[4]  = "CONTROL_ACK",         -- 1 << 2
	[8]  = "CREATE_TYPE_UNK",     -- 1 << 3
	[16] = "CREATE_ALLOC_FAILED", -- 1 << 4
	[17] = "CONTROL_ERROR"        -- Suivant immédiat après 16
}

oan_ctrl_resp = Proto("osstctrlresp", "OAN Control Response")

oan_resp_code    = ProtoField.uint8("oan.ctrl.resp.code", "Response Code", base.HEX)
oan_resp_channel = ProtoField.uint8("oan.ctrl.resp.channel", "Channel", base.DEC)
oan_resp_err_msg = ProtoField.string("oan.ctrl.resp.err_msg", "Error Message")

oan_ctrl_resp.fields = {
	oan_resp_code,
	oan_resp_channel,
	oan_resp_err_msg
}

function oan_ctrl_resp.dissector(buffer, pinfo, tree)
	local length = buffer:len()
	if length == 0 then return end

	local resp_val = buffer(0, 1):le_uint()
	local channel  = buffer(1, 1):le_uint()

	local resp_name = resp_codes_valstr[resp_val] or "Unknown"

	pinfo.cols.protocol = "OAN Control Response"
	pinfo.cols.info:append(string.format("OAN Control Response: %s (Channel: %d)", resp_name, channel))

	local subtree = tree:add(oan_ctrl_resp, buffer(), "OAN Control Response")

	subtree:add_le(oan_resp_code,    buffer(0, 1))
	subtree:add_le(oan_resp_channel, buffer(1, 1))
	subtree:add_le(oan_resp_err_msg, buffer(2, 64))
end
