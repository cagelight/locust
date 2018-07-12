#include "locust/locust.hh"

#include <botan/hash.h>
#include <botan/base64.h>

using namespace locust;

static std::unique_ptr<Botan::HashFunction> hash_sha1 { Botan::HashFunction::create("SHA-1") };
static std::mutex hash_lk;

#define TBREAK { sig.m = asterid::cicada::server::signal::mask::terminate; return sig; }
#define EBREAK goto end;

exchange::~exchange() {
	if (websocket_interface_) {
		std::lock_guard<std::mutex> lkg(websocket_interface_->lk);
		websocket_interface_->alive.store(false);
	}
}

asterid::cicada::server::signal protocol_base::ready(asterid::cicada::connection & con, asterid::cicada::server::detail const &) {
	
	asterid::cicada::server::signal sig;

	if (state_ != state::terminate_on_write) {
		if (con.read(work_in) < 0) {
			sig.m = asterid::cicada::server::signal::mask::terminate;
			return sig;
		}
		
		if (!work_in.size()) sig.m |= asterid::cicada::server::signal::mask::wait_for_read;
	}
	
	while (true) {
		switch (state_) {
			case state::request_header_read: {
				switch (req_head.parse(work_in)) {
					case http::request_header::parse_status::invalid:
						TBREAK
					case http::request_header::parse_status::incomplete:
						sig.m |= asterid::cicada::server::signal::mask::wait_for_read;
						EBREAK
					case http::request_header::parse_status::complete:
						current_session = session();
						header_good = current_session->process_header(&req_head);
						
						if (req_head.field("Connection") == "Upgrade" && header_good) {
							if (req_head.field("Upgrade") == "websocket") {
								if (current_session->websocket_accept()) {
									
									res_head.clear();
									res_head.code = http::status_code::switching_protocols;
									res_head.fields["Sec-WebSocket-Protocol"] = req_head.field("Sec-WebSocket-Protocol");
									res_head.fields["Connection"] = std::string("Upgrade");
									res_head.fields["Upgrade"] = "websocket";
									
									current_session->websocket_interface_.reset(new websocket_interface {this});
									
									std::string const & key = req_head.field("Sec-WebSocket-Key");
									if (!key.empty()) {
										
										std::string accept = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
										hash_lk.lock();
										auto hash = hash_sha1->process(accept);
										hash_lk.unlock();
										accept = Botan::base64_encode(hash);
										
										res_head.fields["Sec-WebSocket-Accept"] = std::move(accept);
									}
									
									res_head.serialize(work_out);
									current_session->websocket_handle(current_session->websocket_interface_);
									begin_state(state::websocket_run);
									continue;
								}
							}
						}
						
						if (req_head.content_length() && header_good)
							begin_state(state::request_body_read);
						else begin_state(state::response_process);
						continue;
				}
				break;
			}
			case state::request_body_read: {
				asterid::buffer_assembly segment {};
				read_counter -= work_in.transfer_to(segment, read_counter);
				current_session->body_segment(segment);
				if (read_counter > 0) {
					sig.m |= asterid::cicada::server::signal::mask::wait_for_read;
					EBREAK
				}
				begin_state(state::response_process);
				continue;
			}
			case state::response_process: {
				asterid::buffer_assembly res_body {};
				current_session->process(res_head, res_body);
				res_head.fields["Content-Length"] = std::to_string(res_body.size());
				res_head.fields["Server"] = "locust";
				res_head.serialize(work_out);
				if (res_body.size()) res_body.transfer_to(work_out);
				
				begin_state(header_good ? state::request_header_read : state::terminate_on_write);
				continue;
			}
			case state::websocket_run: {
				sig.m = asterid::cicada::server::signal::mask::wait_for_read;
				switch (ws_frame.parse(work_in)) {
					case websocket_frame::parse_status::invalid:
						TBREAK
					case websocket_frame::parse_status::not_started: {
						std::lock_guard<std::mutex> lkg { current_session->websocket_interface_->lk };
						websocket_frame::create_ping(work_out);
						EBREAK
					}
					case websocket_frame::parse_status::incomplete:
						EBREAK
					case websocket_frame::parse_status::complete: {
						switch (ws_frame.op()) {
							case websocket_frame::opcode::continuation:
							case websocket_frame::opcode::close:
							case websocket_frame::opcode::invalid:
								TBREAK
							case websocket_frame::opcode::pong:
								break;
							case websocket_frame::opcode::ping: {
								std::lock_guard<std::mutex> lkg { current_session->websocket_interface_->lk };
								websocket_frame::create_pong(ws_frame.body(), work_out);
								break;
							}
							case websocket_frame::opcode::text:
								current_session->websocket_message(ws_frame.body(), true);
								break;
							case websocket_frame::opcode::binary:
								current_session->websocket_message(ws_frame.body(), false);
								break;
						}
						ws_frame.reset();
						break;
					}
				}
				break;
			}
			case state::terminate_on_write:
				break;
		}
		break;
	}
	
	end:
	
	if (state_ == state::websocket_run) {
		std::lock_guard<std::mutex> lkg { current_session->websocket_interface_->lk };
		if (con.write_consume(work_out) < 0) TBREAK
		if (work_out.size()) sig.m |= asterid::cicada::server::signal::mask::wait_for_write;
	} else {
		if (con.write_consume(work_out) < 0) TBREAK
		if (work_out.size()) sig.m |= asterid::cicada::server::signal::mask::wait_for_write;
		else if (state_ == state::terminate_on_write) sig.m = asterid::cicada::server::signal::mask::terminate;
	}
	
	return sig;
}

void protocol_base::begin_state(state s) {
	state_ = s;
	switch (state_) {
		case state::request_header_read:
			req_head.clear();
			break;
		case state::request_body_read:
			read_counter = req_head.content_length();
			break;
		case state::response_process:
			res_head.clear();
			break;
		case state::websocket_run:
			ws_frame.reset();
			break;
		case state::terminate_on_write:
			break;
	}
}
