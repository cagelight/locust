#include "locust/locust.hh"

using namespace locust;

#define FAILCHECK if (we == workbuf.end()) return parse_status::invalid;

http::request_header::parse_status http::request_header::parse(asterid::buffer_assembly & buf) {
	
	asterid::buffer_assembly::const_iterator bufi = buf.begin();
	int tcount = 0;
	for (; bufi != buf.end(); bufi++) {
		switch (tcount) {
			case 0:
			case 2:
				if (*bufi == '\r') tcount++;
				else tcount = 0;
				break;
			case 1:
			case 3:
				if (*bufi == '\n') tcount++;
				else tcount = 0;
				break;
		}
		if (tcount == 4) break;
	}
	if (bufi == buf.end()) return parse_status::incomplete;
	
	bufi++;
	asterid::buffer_assembly workbuf;
	buf.transfer_to(workbuf, bufi - buf.begin());
	
	asterid::buffer_assembly::iterator ws = workbuf.begin();
	asterid::buffer_assembly::iterator we = ws;
	
	for(we = ws; we != workbuf.end() && *we != ' '; we++) {} FAILCHECK
	method = asterid::buffer_assembly(ws, we).to_string();
	
	we++; FAILCHECK
	for(ws = we; we != workbuf.end() && *we != ' '; we++) {} FAILCHECK
	path = asterid::buffer_assembly(ws, we).to_string();
	
	if (!path.size()) return parse_status::invalid;
	std::vector<std::string> path_argsplit = asterid::strop::separate(path, '?', 1);
	path = path_argsplit[0];
	if (path_argsplit.size() != 1) {
		std::vector<std::string> args = asterid::strop::separate(path_argsplit[1], '&');
		for (std::string argset : args) {
			std::vector<std::string> arg = asterid::strop::separate(argset, '=', 1);
			if (arg.size() != 2) continue;
			this->args[arg[0]] = arg[1];
		}
	}
	
	we++; FAILCHECK
	for(ws = we; we != workbuf.end() && *we != '\r'; we++) {} FAILCHECK
	std::string version = asterid::buffer_assembly(ws, we).to_string();
	if (version != "HTTP/1.1") return parse_status::invalid;
	
	while (true) {
		we++; FAILCHECK
		we++; FAILCHECK
		if (*we == '\r') break;
		for(ws = we; we != workbuf.end() && *we != ':'; we++) {} FAILCHECK
		std::string key = asterid::buffer_assembly(ws, we).to_string();
		we++; FAILCHECK
		we++; FAILCHECK
		for(ws = we; we != workbuf.end() && *we != '\r'; we++) {} FAILCHECK
		fields[{key.begin(), key.end()}] = asterid::buffer_assembly(ws, we).to_string();;
	}
	
	return parse_status::complete;
}

void http::request_header::serialize(asterid::buffer_assembly &) {
	
}

void http::request_header::clear() {
	method.clear();
	path.clear();
	
	args.clear();
	fields.clear();
	cookies.clear();
}
