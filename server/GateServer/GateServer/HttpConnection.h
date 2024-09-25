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
	//��������
	beast::flat_buffer _buffer{ 8192 };
	//��������
	http::request<http::dynamic_body> _request;
	//��Ӧ�ͻ���
	http::response<http::dynamic_body> _response;
	//��ʱ��
	net::steady_timer deadline_{
		_socket.get_executor(), std::chrono::seconds(60)
	};
	//�洢url
	std::string _get_url;
	//�洢����
	std::unordered_map<std::string, std::string> _get_params;
};