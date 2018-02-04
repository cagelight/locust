#include "locust/locust.hh"

using namespace locust;

void http::response_header::serialize(asterid::buffer_assembly & buf) {
	
	buf << "HTTP/1.1 ";
	buf << std::to_string(static_cast<uint_fast16_t>(code)) << " " << status_text(code);
	buf << "\r\n";
	for (auto const & i : fields) 
		buf << i.first.c_str() << ": " << i.second.c_str() << "\r\n";
	buf << "\r\n";
}

void http::response_header::clear() {
	fields.clear();
}
