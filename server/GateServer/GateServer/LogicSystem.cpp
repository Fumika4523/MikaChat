#include "LogicSystem.h"
#include "HttpConnection.h"

LogicSystem::LogicSystem() {
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test req";
		int i = 0;
		for (auto& elem : connection->_get_params) {
			i++;
			beast::ostream(connection->_response.body()) << "param" << i << " key is " << elem.first;
			beast::ostream(connection->_response.body()) << ", value is " << elem.second << std::endl;
		}
	});
}


void LogicSystem::RegGet(std::string url, HttpHandler handler) {
	_get_handlers.insert(make_pair(url, handler));
}

bool LogicSystem::HandleGet(std::string url, std::shared_ptr<HttpConnection> con) {
	if (_get_handlers.find(url) == _get_handlers.end()) {
		return false;
	}

	_get_handlers[url](con);
	return true;
}