#include "locust/locust.hh"

#include <botan/hash.h>
#include <botan/base64.h>

using namespace locust;

static std::unique_ptr<Botan::HashFunction> hash_sha1 { Botan::HashFunction::create("SHA-1") };
static std::mutex hash_lk;

#define TBREAK { sig.m = asterales::cicada::reactor::signal::mask::terminate; return sig; }
#define EBREAK goto end;

client_exchange::~client_exchange() {
	/*
	if (websocket_interface_) {
		std::lock_guard<std::mutex> lkg(websocket_interface_->lk);
		websocket_interface_->alive.store(false);
	}
	*/
}

asterales::cicada::reactor::signal client_protocol_base::ready(asterales::cicada::connection & con, asterales::cicada::reactor::detail const &) {
	
	asterales::cicada::reactor::signal sig;

	if (state_ != state::terminate) {
		if (con.read(work_in) < 0) TBREAK;
		if (!work_in.size()) sig.m |= asterales::cicada::reactor::signal::mask::wait_for_read;
	}
	
	while (true) {
		switch (state_) {
		case state::request_process:
			break;
		case state::response_header_read:
			break;
		case state::response_body_read:
			break;
		case state::terminate:
			break;
		}
		break;
	}
	
	end:
	
	if (con.write_consume(work_out) < 0) TBREAK
	if (work_out.size()) sig.m |= asterales::cicada::reactor::signal::mask::wait_for_write;
	
	return sig;
}

void client_protocol_base::begin_state(state s) {
	state_ = s;
	switch(state_) {
	case state::request_process:
		req_head.clear();
		break;
	case state::response_header_read:
		res_head.clear();
		break;
	case state::response_body_read:
		read_counter = res_head.content_length();
		break;
	case state::terminate:
		break;
	}
}
