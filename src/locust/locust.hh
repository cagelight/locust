#pragma once

#include <asterales/cicada.hh>
#include <asterales/strop.hh>
#include <asterales/istring.hh>

#include <sstream>
#include <string_view>
#include <unordered_map>

namespace locust {
	
	namespace http {
	
		typedef std::unordered_map<asterales::istring, std::string> field_map;
		typedef std::unordered_map<std::string, std::string> arg_map;
		typedef std::unordered_map<std::string, std::string> cookie_map;
		
		static std::string const default_mime {"application/octet-stream"};
		
		typedef uint_fast16_t status_code_t;
		enum struct status_code : status_code_t {
			// 1XX
			continue_s = 100,
			switching_protocols = 101,
			processing = 102,
			
			// 2XX
			ok = 200,
			created = 201,
			accepted = 202,
			non_authorative_information = 203,
			no_content = 204,
			reset_content = 205,
			partial_content = 206,
			multi_status = 207,
			already_reported = 208,
			im_used = 226,
			
			// 3XX
			multiple_choices = 300,
			moved_permanently = 301,
			found = 302,
			see_other = 303,
			not_modified = 304,
			use_proxy = 305,
			switch_proxy = 306,
			temporary_redirect = 307,
			permanent_redirect = 308,
			
			// 4XX
			bad_request = 400,
			unauthorized = 401,
			payment_required = 402,
			forbidden = 403,
			not_found = 404,
			method_not_allowed = 405,
			not_acceptable = 406,
			proxy_authentication_required = 407,
			request_timeout = 408,
			conflict = 409,
			gone = 410,
			length_required = 411,
			precondition_failed = 412,
			payload_too_large = 413,
			uri_too_long = 414,
			unsupported_media_type = 415,
			range_not_satisfiable = 416,
			expectation_failed = 417,
			im_a_teapot = 418,
			misdirected_request = 421,
			unprocessable_entity = 422,
			locked = 423,
			failed_dependency = 424,
			upgrade_required = 426,
			precondition_required = 428,
			too_many_requests = 429,
			request_header_fields_too_large = 431,
			unavailable_for_legal_reasons = 451,
			
			// 5XX
			internal_server_error = 500,
			not_implemented = 501,
			bad_gateway = 502,
			service_unavailable = 503,
			gateway_timeout = 504,
			http_version_not_supported = 505,
			variant_also_negotiates = 506,
			insufficient_storage = 507,
			loop_detected = 508,
			not_extended = 510,
			network_authentication_required = 511,
		};
		
		char const * status_text(status_code stat);
		
		struct request_header {
			
			enum struct parse_status {
				invalid,
				incomplete,
				complete
			};
			
			std::string method; // GET, POST, etc.
			std::string path;
			arg_map args;
			field_map fields;
			cookie_map cookies;
			
			request_header() = default;
			
			parse_status parse(asterales::buffer_assembly & buf); // will consume from buffer if complete, otherwise does not consume
			void serialize(asterales::buffer_assembly & buf); // append to end
			void clear();
			
			inline std::string const & field(asterales::istring const & f) const {
				auto i = fields.find(f);
				if (i != fields.end()) return i->second;
				return asterales::empty_str;
			}
			
			inline std::string const & argument(std::string const & f) const {
				auto i = args.find(f);
				if (i != args.end()) return i->second;
				return asterales::empty_str;
			}

			inline std::string const & cookie(std::string const & f) const {
				auto i = cookies.find(f);
				if (i != cookies.end()) return i->second;
				return asterales::empty_str;
			}
			
			inline size_t content_length() const {
				std::string cstr = field("Content-Length");
				if (!cstr.size()) return 0;
				else return strtoul(cstr.c_str(), nullptr, 10);
			}
			
			inline std::string const & content_type() const {
				return field("Content-Type");
			}
		};
		
		struct response_header {
			
			status_code code = status_code::internal_server_error;
			field_map fields;
			
			response_header() = default;
			
			void serialize(asterales::buffer_assembly & buf); // append to end
			void clear();
			
			inline std::string const & field(asterales::istring const & f) const {
				auto i = fields.find(f);
				if (i != fields.end()) return i->second;
				return asterales::empty_str;
			}
			
			inline size_t content_length() const {
				std::string cstr = field("Content-Length");
				if (!cstr.size()) return 0;
				else return strtoul(cstr.c_str(), nullptr, 10);
			}
			
			inline std::string const & content_type() const {
				return field("Content-Type");
			}
		};
	
	}
	
	// ================================================================
	// ----------------------------------------------------------------
	// ================================================================
	
	struct websocket_frame {
		
		enum struct parse_status {
			invalid,
			not_started,
			incomplete,
			complete
		};
		
		enum struct opcode : uint8_t {
			continuation,
			text,
			binary,
			close,
			ping,
			pong,
			invalid
		};
		
		parse_status parse(asterales::buffer_assembly & buf);
		void reset();
		
		asterales::buffer_assembly const & body() const { return bodybuf; }
		opcode op() const { return opcode_; }
		
		static void create_ping(asterales::buffer_assembly & out_buffer);
		static void create_pong(asterales::buffer_assembly const & body, asterales::buffer_assembly & out_buffer);
		static void create_msg(asterales::buffer_assembly const & body, bool is_text, asterales::buffer_assembly & out_buffer);
		
	private:
		
		enum struct parse_state {
			first_2,
			payload_length,
			mask,
			payload_body,
			done
		} parse_state_;
		
		asterales::buffer_assembly workbuf, bodybuf;
		bool fin;
		opcode opcode_;
		size_t payload_length;
		bool has_mask;
		uint32_t mask;
		
	};
	
	// ================================================================
	// ----------------------------------------------------------------
	// ================================================================
	
	struct server_websocket_interface;
	typedef std::shared_ptr<server_websocket_interface> sv_wsi_ptr;
	
	struct server_exchange {
		virtual ~server_exchange();
		virtual bool process_header(http::request_header const *) = 0;
		virtual void body_segment(asterales::buffer_assembly const &) = 0;
		virtual void process(http::response_header & header, asterales::buffer_assembly & body) = 0;
		virtual bool websocket_accept() = 0;
		virtual void websocket_handle(sv_wsi_ptr) = 0;
		virtual void websocket_message(asterales::buffer_assembly const & data, bool text) = 0;
		
		std::shared_ptr<server_websocket_interface> websocket_interface_;
	};
	
	struct dummy_exchange : public server_exchange {
		virtual bool process_header(http::request_header const *) override { return false; }
		virtual void body_segment(asterales::buffer_assembly const &) override {  }
		virtual void process(http::response_header & header, asterales::buffer_assembly & body) override {
			body << "dummy";
			header.fields["Content-Type"] = "text/plain";
		}
		virtual bool websocket_accept() override { return false; };
		virtual void websocket_handle(sv_wsi_ptr) override { };
		virtual void websocket_message(asterales::buffer_assembly const &, bool) override {}
	};
	
	struct basic_exchange : public dummy_exchange {
		virtual bool process_header(locust::http::request_header const * header) override {
			req_head = header;
			return true;
		}
		virtual void body_segment(asterales::buffer_assembly const & buf) override {
			req_body << buf;
		}
		virtual void process(http::response_header & res_head, asterales::buffer_assembly & res_body) override = 0;
		
		http::request_header const * req_head;
		asterales::buffer_assembly req_body;
	};
	
	struct server_protocol_base : public asterales::cicada::reactor::protocol {
		virtual ~server_protocol_base() = default;
		asterales::cicada::reactor::signal ready(asterales::cicada::connection & con, asterales::cicada::reactor::detail const & d) override final;
	protected:
		virtual std::unique_ptr<server_exchange> session() = 0;
	private:
		enum struct state {
			request_header_read,
			request_body_read,
			response_process,
			websocket_run,
			terminate_on_write
		} state_ = state::request_header_read;
		asterales::buffer_assembly work_in {};
		asterales::buffer_assembly work_out {};
		ssize_t read_counter = 0;
		http::request_header req_head;
		http::response_header res_head;
		websocket_frame ws_frame;
		std::unique_ptr<server_exchange> current_session;
		std::shared_ptr<server_websocket_interface> wsi;
		bool header_good;
		
		void begin_state(state);
		
		friend server_websocket_interface;
	};
	
	template <typename T> struct server_protocol : public server_protocol_base {
	protected:
		virtual std::unique_ptr<server_exchange> session() override { return std::unique_ptr<server_exchange> { new T {} }; }
	};
	
	struct server_websocket_interface {
		server_websocket_interface(server_protocol_base * parent) : parent(parent) {}
		void send(std::string const &);
		inline bool is_alive() { return alive; }
	private:
		server_protocol_base * parent;
		std::atomic_bool alive {true};
		std::mutex lk;
		
	friend server_exchange;
	friend server_protocol_base;
	};
	
	// ================================================================
	// ----------------------------------------------------------------
	// ================================================================
	
	struct client_exchange {
		virtual ~client_exchange();
		virtual void process(http::request_header & header, asterales::buffer_assembly & body) = 0;
		virtual bool process_header(http::response_header const *) = 0;
		virtual void body_segment(asterales::buffer_assembly const &) = 0;
		/*
		virtual bool websocket_connect() = 0;
		virtual void websocket_handle(sv_wsi_ptr) = 0;
		virtual void websocket_message(asterales::buffer_assembly const & data, bool text) = 0;
		
		std::shared_ptr<server_websocket_interface> websocket_interface_;
		*/
	};
	
	struct client_protocol_base : public asterales::cicada::reactor::protocol {
		virtual ~client_protocol_base() = default;
		asterales::cicada::reactor::signal ready(asterales::cicada::connection & con, asterales::cicada::reactor::detail const & d) override final;
	protected:
		virtual std::unique_ptr<client_exchange> session() = 0;
	private:
		enum struct state {
			request_process,
			response_header_read,
			response_body_read,
			//websocket_run,
			terminate
		} state_ = state::request_process;
		asterales::buffer_assembly work_in {};
		asterales::buffer_assembly work_out {};
		http::request_header req_head;
		http::response_header res_head;
		ssize_t read_counter = 0;
		
		void begin_state(state);
	};
	
}
