#include "locust/locust.hh"
#include <byteswap.h>

#include <botan/auto_rng.h>

static Botan::AutoSeeded_RNG rng;
static std::mutex rng_lk;

using namespace locust;

// the order of these are reversed for some reason? I can't figure it out, probably something weird with network bit order

struct byte_1 {
	uint8_t opcode : 4;
	uint8_t rsv3 : 1;
	uint8_t rsv2 : 1;
	uint8_t rsv1 : 1;
	uint8_t fin : 1;
};

struct byte_2 {
	uint8_t payload_len : 7;
	uint8_t mask : 1;
};

void websocket_frame::reset() {
	parse_state_ = parse_state::first_2;
	workbuf.clear();
	mask = 0;
}

void websocket_frame::create_ping(asterid::buffer_assembly & out_buffer) {
	byte_1 b1;
	byte_2 b2;
	
	b1.opcode = 0x09;
	b1.rsv1 = 0;
	b1.rsv2 = 0;
	b1.rsv3 = 0;
	b1.fin = 1;
	
	b2.mask = 0;
	b2.payload_len = 32;
	
	out_buffer.write<byte_1>(b1);
	out_buffer.write<byte_2>(b2);
	
	rng_lk.lock();
	auto dat = rng.random_vec(32);
	rng_lk.unlock();
	
	out_buffer.write(dat.data(), 32);
}

void websocket_frame::create_pong(asterid::buffer_assembly const & body, asterid::buffer_assembly & out_buffer) {
	byte_1 b1;
	byte_2 b2;
	
	b1.opcode = 0x0A;
	b1.rsv1 = 0;
	b1.rsv2 = 0;
	b1.rsv3 = 0;
	b1.fin = 1;
	
	b2.mask = 0;
	b2.payload_len = body.size();
	
	out_buffer.write<byte_1>(b1);
	out_buffer.write<byte_2>(b2);
	
	out_buffer << body;
}

void websocket_frame::create_msg(asterid::buffer_assembly const & body, bool is_text, asterid::buffer_assembly & out_buffer) {
	byte_1 b1;
	byte_2 b2;
	
	b1.opcode = is_text ? 0x01 : 0x02;
	b1.rsv1 = 0;
	b1.rsv2 = 0;
	b1.rsv3 = 0;
	b1.fin = 1;
	
	out_buffer.write<byte_1>(b1);
	
	b2.mask = 0;
	
	size_t payload_len = body.size();
	if (payload_len <= 125) {
		b2.payload_len = payload_len;
		out_buffer.write<byte_2>(b2);
	} else if (payload_len <= 0xFFFF) {
		b2.payload_len = 126;
		out_buffer.write<byte_2>(b2);
		out_buffer.write<uint16_t>(bswap_16(payload_len));
	} else {
		b2.payload_len = 127;
		out_buffer.write<byte_2>(b2);
		out_buffer.write<uint64_t>(bswap_64(payload_len));
	}
	
	out_buffer << body;
}

websocket_frame::parse_status websocket_frame::parse(asterid::buffer_assembly & buf) {
	
	switch (parse_state_) {
		case parse_state::first_2: {
			if (!buf.precheck(2)) return parse_status::not_started;
			byte_1 b1 = buf.read<byte_1>();
			byte_2 b2 = buf.read<byte_2>();
			fin = b1.fin;
			switch (b1.opcode) {
				default: opcode_ = opcode::invalid; break;
				case 0x00: opcode_ = opcode::continuation; break;
				case 0x01: opcode_ = opcode::text; break;
				case 0x02: opcode_ = opcode::binary; break;
				case 0x08: opcode_ = opcode::close; break;
				case 0x09: opcode_ = opcode::ping; break;
				case 0x0A: opcode_ = opcode::pong; break;
			}
			if (opcode_ != opcode::continuation) bodybuf.clear();
			payload_length = b2.payload_len;
			has_mask = b2.mask;
			parse_state_ = parse_state::payload_length;
		}
		[[fallthrough]]; case parse_state::payload_length:
			if (payload_length == 126) {
				if (!buf.precheck<uint16_t>()) return parse_status::incomplete;
				payload_length = bswap_16(buf.read<uint16_t>());
			} else if (payload_length == 127) {
				if (!buf.precheck<uint64_t>()) return parse_status::incomplete;
				payload_length = bswap_64(buf.read<uint64_t>());
			}
			parse_state_ = parse_state::mask;
		[[fallthrough]]; case parse_state::mask:
			if (has_mask) {
				if (!buf.precheck<uint32_t>()) return parse_status::incomplete;
				mask = buf.read<uint32_t>();
			}
			parse_state_ = parse_state::payload_body;
		[[fallthrough]]; case parse_state::payload_body:
			if (payload_length > 0) payload_length -= buf.transfer_to(workbuf, payload_length);
			if (payload_length > 0) return parse_status::incomplete;
			if (has_mask) {
				payload_length = workbuf.size();
				if (has_mask) for (size_t i = 0; i < payload_length; i++) workbuf[i] ^= reinterpret_cast<uint8_t *>(&mask)[i % 4];
			}
			bodybuf << workbuf;
			parse_state_ = parse_state::done;
		[[fallthrough]]; case parse_state::done:
			if (!fin) {
				parse_state_ = parse_state::first_2;
				return parse_status::incomplete;
			}
			return parse_status::complete;
	}
	
	return parse_status::invalid;
}

void server_websocket_interface::send(std::string const & str) {
	std::lock_guard<std::mutex> lkg(lk);
	if (!alive) return;
	asterid::buffer_assembly buf;
	buf << str;
	asterid::buffer_assembly obuf;
	websocket_frame::create_msg(buf, true, obuf);
	parent->work_out << obuf;
	parent->set_mask(asterid::cicada::reactor::signal::mask::wait_for_write | asterid::cicada::reactor::signal::mask::wait_for_read);
}
