#include "locust/locust.hh"

char const * locust::http::status_text(status_code stat) {
	switch (stat) {
		case status_code::continue_s:
			return "Continue";
		case status_code::switching_protocols:
			return "Switching Protocols";
		// ...
		case status_code::ok:
			return "OK";
		case status_code::created:
			return "Created";
		case status_code::accepted:
			return "Accepted";
		case status_code::non_authorative_information:
			return "Non-Authorative Information";
		case status_code::no_content:
			return "No Content";
		case status_code::reset_content:
			return "Reset Content";
		case status_code::partial_content:
			return "Partial Content";
		// ...
		case status_code::see_other:
			return "See Other";
		case status_code::not_modified:
			return "Not Modified";
		// ...
		case status_code::bad_request:
			return "Bad Request";
		case status_code::unauthorized:
			return "Unauthorized";
		case status_code::payment_required:
			return "Payment Required";
		case status_code::forbidden:
			return "Forbidden";
		case status_code::not_found:
			return "Not Found";
		case status_code::method_not_allowed:
			return "Method Not Allowed";
		case status_code::not_acceptable:
			return "Not Acceptable";
		case status_code::proxy_authentication_required:
			return "Proxy Authentication Required";
		case status_code::request_timeout:
			return "Request Timeout";
		case status_code::conflict:
			return "Conflict";
		case status_code::gone:
			return "Gone";
		case status_code::length_required:
			return "Length Required";
		case status_code::precondition_failed:
			return "Precondition Failed";
		case status_code::payload_too_large:
			return "Payload Too Large";
		case status_code::uri_too_long:
			return "URI Too Long";
		case status_code::unsupported_media_type:
			return "Unsupported Media Type";
		case status_code::range_not_satisfiable:
			return "Range Not Satisfiable";
		case status_code::expectation_failed:
			return "Expectation Failed";
		case status_code::im_a_teapot:
			return "I'm a teapot";
		case status_code::misdirected_request:
			return "Misdirected Request";
		case status_code::unprocessable_entity:
			return "Unprocessable Entity";
		case status_code::locked:
			return "Locked";
		case status_code::failed_dependency:
			return "Failed Dependency";
		case status_code::upgrade_required:
			return "Upgrade Required";
		case status_code::precondition_required:
			return "Precondition Required";
		case status_code::too_many_requests:
			return "Too Many Requests";
		case status_code::request_header_fields_too_large:
			return "Request Header Fields Too Large";
		case status_code::unavailable_for_legal_reasons:
			return "Unavailable For Legal Reasons";
		case status_code::internal_server_error:
			return "Internal Server Error";
		case status_code::not_implemented:
			return "Not Implemented";
		case status_code::bad_gateway:
			return "Bad Gateway";
		// ...
		default:
			return "Unknown Status Code";
	}
}
