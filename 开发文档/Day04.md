# beast搭建http服务器

### 绑定和监听连接

使用visual studio创建项目GateServer，用vcpkg配置boost库和jsoncpp，创建const.h将文件和一些作用域声明放在const.h里，简化头文件

~~~c++
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
~~~

添加一个新的类，名字叫CServer，在头文件中声明acceptor，以及用于事件循环的上下文iocontext,和构造函数

~~~c++
#include "const.h"

class CServer:public std::enable_shared_from_this<CServer>
{
public:
	CServer(boost::asio::io_context& ioc, unsigned short port);
	void Start();
private:
	tcp::acceptor _acceptor;
	net::io_context& _ioc;
	tcp::socket _socket;
};
~~~

cpp中实现构造函数

~~~c++
CServer::CServer(boost::asio::io_context& ioc, unsigned short port) :  _ioc(ioc), 
_acceptor(ioc, tcp::endpoint(tcp::v4(), port)), _socket(ioc) {
	
}
~~~

接下来实现Start函数，用来监听新链接

~~~c++
void CServer::Start() {
	auto self = shared_from_this();
	_acceptor.async_accept(_socket, [self](beast::error_code ec) {
		try {
			//出错则放弃这个连接，继续监听新连接
			if (ec) {
				self->Start();
				return;
			}

			//处理新连接，创建HttpConnection类管理连接
			std::make_shared<HttpConnection>(std::move(self->_socket))->Start();
			//继续监听
			self->Start();
		}
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
			self->Start();
		}
	});
}
~~~

Start函数内创建HttpConnection类型智能指针，将_socket内部数据转移给HttpConnection管理，_socket继续用来接受写的链接。

新建HttpConnection类文件，在头文件添加声明

~~~c++
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
};
~~~

实现HttpConnection构造函数

~~~c++
HttpConnection::HttpConnection(tcp::socket socket)
    : _socket(std::move(socket)) {
}
~~~

实现Start函数

~~~c++
void HttpConnection::Start() {
	auto self = shared_from_this();
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec,
		std::size_t bytes_transferred) {
			try {
				if (ec) {
					std::cout << "http read err is " << ec.what() << std::endl;
					return;
				}

				//处理读到的数据
				boost::ignore_unused(bytes_transferred);
				self->HandleReq();
				self->CheckDeadline();
			}
			catch (std::exception& exp) {
				std::cout << "exception is " << exp.what() << std::endl;
			}
		});
}
~~~

实现HandleReq

~~~c++
void HttpConnection::HandleReq() {
	//设置版本
	_request.version(_request.version());
	//设置短链接
	_request.keep_alive(false);

	if (_request.method() == http::verb::get) {
		bool success = LogicSystem::GetInstance()->HandleGet(_request.target(), shared_from_this());
		if (!success) {
			//失败返回404
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}

		//成功返回作用域
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}
}
~~~

先实现Get请求的处理，根据请求类型为get调用LogicSystem的HandleGet接口处理get请求，根据处理成功还是失败回应数据包给对方。
~~~c++
void HttpConnection::HandleReq() {
	//设置版本
	_request.version(_request.version());
	//设置短链接
	_request.keep_alive(false);

	if (_request.method() == http::verb::get) {
		bool success = LogicSystem::GetInstance()->HandleGet(_request.target(), shared_from_this());
		if (!success) {
			//失败返回404
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}

		//成功返回作用域
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}
}
~~~

LogicSystem采用单例模式，单例基类如下

~~~c++
#include <memory>
#include <mutex>
#include <iostream>
template <typename T>
class Singleton {
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>& st) = delete;

    static std::shared_ptr<T> _instance;
public:
    static std::shared_ptr<T> GetInstance() {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]() {
            _instance = std::shared_ptr<T>(new T);
            });

        return _instance;
    }
    void PrintAddress() {
        std::cout << _instance.get() << std::endl;
    }
    ~Singleton() {
        std::cout << "this is singleton destruct" << std::endl;
    }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;
~~~

const.h中增加文件

~~~c++
#include "Singleton.h"
#include <functional>
#include <map>
#include "const.h"
~~~

实现LogicSystem类
~~~c++
class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;
class LogicSystem:public Singleton<LogicSystem>
{
	friend class Singleton;
public:
	~LogicSystem() {};
	bool HandleGet(std::string url, std::shared_ptr<HttpConnection> con);
	void RegGet(std::string url, HttpHandler handler);
private:
	LogicSystem();
	std::map<std::string, HttpHandler> _post_handlers;
	std::map<std::string, HttpHandler> _get_handlers;
};
~~~

_post_handlers和_get_handlers分别是post请求和get请求的回调函数map，key为路由，value为回调函数。

实现RegGet函数，接受路由和回调函数作为参数

~~~c++
void LogicSystem::RegGet(std::string url, HttpHandler handler) {
	_get_handlers.insert(make_pair(url, handler));
}
~~~

在构造函数中实现具体的消息注册
~~~c++
LogicSystem::LogicSystem() {
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test req";
	});
}
~~~

为防止互相引用，以及LogicSystem能够成功访问HttpConnection，在LogicSystem.cpp中包含HttpConnection头文件

并且在HttpConnection中添加友元类LogicSystem, 且在HttpConnection.cpp中包含LogicSystem.h文件

~~~c++
bool LogicSystem::HandleGet(std::string url, std::shared_ptr<HttpConnection> con) {
	if (_get_handlers.find(url) == _get_handlers.end()) {
		return false;
	}

	_get_handlers[url](con);
	return true;
}
~~~

在HttpConnection实现WriteResponse函数
~~~c++
void HttpConnection::WriteResponse() {
	auto self = shared_from_this();
	_response.content_length(_response.body().size());
	//http是短链接，发送结束后关闭发送端，取消定时器
	http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t) {
		self->_socket.shutdown(tcp::socket::shutdown_send, ec);
		self->deadline_.cancel();
	});
}
~~~

http处理请求需要有一个时间约束，发送的数据包不能超时。所以在发送时启动一个定时器，收到发送的回调后取消定时器

~~~c++
void HttpConnection::CheckDeadline() {
	auto self = shared_from_this();

	deadline_.async_wait([self](beast::error_code ec) {
        // 时限内没有出错，关闭socket
        if (!ec) {
            self->_socket.close();
        }
	});
}
~~~

在主函数中初始化上下文iocontext以及启动信号监听ctr-c退出事件， 并且启动iocontext服务
~~~c++
#include "CServer.h"
int main()
{
    try
    {
        unsigned short port = static_cast<unsigned short>(8080);
        net::io_context ioc{ 1 };
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {

            if (error) {
                return;
            }
            ioc.stop();
            });
        std::make_shared<CServer>(ioc, port)->Start();
        ioc.run();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
~~~

启动服务器，在浏览器输入`http://localhost:8080/get_test`

会看到服务器回包`receive get_test req`

如果我们输入带参数的url请求`http://localhost:8080/get_test?key1=value1&key2=value2`

会收到服务器反馈`url not found`

还需要实现带参数的url解析函数

先实现16进制和十进制互转函数

~~~c++
//char 转为16进制
unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

//16进制转为char
unsigned char FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}
~~~

实现URL编码和解码函数

~~~c++
std::string UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//判断是否仅有数字和字母构成
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //为空字符
			strTemp += "+";
		else
		{
			//其他字符需要提前加%并且高四位和低四位分别转为16进制
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] & 0x0F);
		}
	}
	return strTemp;
}

std::string UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//还原+为空
		if (str[i] == '+') strTemp += ' ';
		//遇到%将后面的两个字符从16进制转为char再拼接
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}
~~~

在HttpConnection中添加存储url的参数的成员变量，和带参数的url解析方法

~~~c++
void PreParseGetParam();

//存储url
std::string _get_url;
//存储参数
std::unordered_map<std::string, std::string> _get_params;
~~~

实现PreParseGetParam

~~~c++
void HttpConnection::PreParseGetParam() {
	//提取URI
	auto uri = _request.target();
	//查询字符串开始的位置（即'?'的位置）
	auto query_pos = uri.find('?');
	if (query_pos == std::string::npos) {
		_get_url = uri;
		return;
	}
	
	_get_url = uri.substr(0, query_pos);
	std::string query_string = uri.substr(query_pos + 1);
	std::string key;
	std::string value;
	size_t pos = 0;
	while ((pos = query_string.find('&')) != std::string::npos) {
		auto pair = query_string.substr(0, pos);
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(pair.substr(0, eq_pos));
			value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;
		}
		query_string.erase(0, pos + 1);
	}
	//处理最后一个参数对（如果没有 & 分隔符）
	if (!query_string.empty()) {
		size_t eq_pos = query_string.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(query_string.substr(0, eq_pos));
			value = UrlDecode(query_string.substr(eq_pos + 1));
			_get_params[key] = value;
		}
	}
}
~~~

HttpConnection::HandleReq函数略作修改
~~~c++
void HttpConnection::HandleReq() {
    //...省略
    if (_request.method() == http::verb::get) {
        PreParseGetParam();
        bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
    }
    //...省略
}
~~~

修改LogicSytem构造函数，在get_test的回调里返回参数给对端

~~~c++
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
~~~

在浏览器输入`http://localhost:8080/get_test?key1=value1&key2=value2`

看到浏览器收到如下图信息，说明get请求逻辑处理完了

![image-20240924223715462](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20240924223715462.png)
