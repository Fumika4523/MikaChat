#pragma once
#include "const.h"

class HttpConnection:public std::enable_shared_from_this<HttpConnection>
{
	friend class LogicSystem;
public:
	HttpConnection(tcp::socket socket);
	void Start();

private:
	void CheckDeadline();
	void WriteResponse();
	void HandleReq();
	void PreParseGetParam();
	tcp::socket _socket;
	//接受数据
	beast::flat_buffer _buffer{ 8192 };
	//解析请求
	http::request<http::dynamic_body> _request;
	//回应客户端
	http::response<http::dynamic_body> _response;
	//定时器
	net::steady_timer deadline_{
		_socket.get_executor(), std::chrono::seconds(60)
	};
	//存储url
	std::string _get_url;
	//存储参数
	std::unordered_map<std::string, std::string> _get_params;
};