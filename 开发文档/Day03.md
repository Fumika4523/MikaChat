### 增加singleton模板单例类

~~~c++
template <typename T>
class Singleton{
protected:
    Singleton() = default;
    Singleton(const Singleton &) = delete;
    Singleton& operator = (const Singleton &) = delete;
    static std::shared_ptr<T> _instance;
public:
    static std::shared_ptr<T> GetInstance(){
        static std::once_flag s_flag;
        std::call_once(s_flag, [&](){
            _instance = std::shared_ptr<T>(new T);
        });
        
        return _instance;
    }
    
    void PrintAddress(){
        std:: cout << _instance.get() << std::endl;
    }
    
    ~Singleton(){
        std::cout << "this is singleton destruct" << std::endl;
    }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;
~~~

> Q：为什么构造函数权限用protected？
>
> A：期望继承模板单例类的子类，可以构造基类
>
> Q：为什么用new，不用make_shared？
>
> A：继承模板单例类的子类的构造会设置为private，make_shared无法访问私有构造函数

### http管理类

在pro中添加网络库

~~~
QT += core gui network
~~~

添加HttpMgr类，头文件如下：

~~~C++
#ifndef HTTPMGR_H
#define HTTPMGR_H
#include "singleton.h"
#include <QString>
#include <QObject>
#include <QUrl>
#include <QNetworkAccessManager>
#include "global.h"
#include <memory>
#include <QJsonObject>
#include <QJsonDocument>


class HttpMgr:public Singleton<HttpMgr>
{
    Q_OBJECT
    
public:
    ~HttpMgr();
private:
    friend class Singleton<HttpMgr>;
    HttpMgr();
    QNetworkAccessManager _manager;
    void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod);
signals:
    void sig_http_finish();
};

#endif // HTTPMGR_H

~~~

实现PostHttpReq函数，也就是发送http的post请求，需要以下参数：请求的url，请求的数据，请求的id，以及哪个模块发出的请求mod

~~~C++
void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod);
~~~

在global中定义以下枚举类型

~~~c++
//请求id
enum ReqId{
    ID_GET_VERIFY_CODE = 1001,
    ID_REG_USER = 1002,
};
//错误代码
enum ErrorCode{
    SUCCESS = 0,
    ERR_JSON = 1,
    ERR_NETWORK = 2,
};
//模块
enum Modules{
    REGISTERMOD = 0,
};
~~~

还需要增加发送信号的参数

~~~c++
void sig_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);
~~~

PostHttpReq实现如下

~~~c++
void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod)
{
    //转换json数据为QByteArray
    QByteArray data = QJsonDocument(json).toJson();
    //通过url构造HTTP请求，设置请求头和请求体
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));
    //获取自己的智能指针，构造伪闭包
    auto self = shared_from_this();
    //发送post请求
    QNetworkReply* reply = _manager.post(request, data);
    //设置信号和槽等待发送完成
    QObject::connect(reply, &QNetworkReply::finished, [reply, self, req_id, mod](){
        //处理错误情况
        if (reply->error() != QNetworkReply::NoError){
            qDebug() << reply->errorString();
            //发送信号通知完成
            emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
            reply->deleteLater();
            return;
        }

        //无错误则读回请求
        QString res = reply->readAll();

        //发送信号通知完成
        emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);
        reply->deleteLater();
        return;
    });
}
~~~

添加一个slot_http_finish槽函数，来接受sig_http_finish信号

~~~c++
void HttpMgr::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod)
{
    if (mod == Modules::REGISTERMOD){
        emit sig_reg_mod_finish(id, res, err);
    }
}
~~~

添加注册模块http请求完成时发送的信号sig_reg_mod_finish

~~~c++
class HttpMgr:public QObject, public Singleton<HttpMgr>,
        public std::enable_shared_from_this<HttpMgr>
{
    Q_OBJECT

public:
   //...省略
signals:
    void sig_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);
    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes err);
};
~~~

在cpp文件中连接sig_http_finish和slot_http_finish

~~~c++
HttpMgr::HttpMgr()
{
    //连接http请求和完成信号
    connect(this, &HttpMgr::sig_http_finish, this, &HttpMgr::slot_http_finish);
}
~~~

在注册界面连接sig_reg_mod_finish信号

~~~c++
RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegisterDialog)
{
    ...
    
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reg_mod_finish, this, &RegisterDialog::slot_reg_mod_finish);
}
~~~

实现slot_reg_mod_finish函数

~~~c++
void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"), false);
        return;
    }
    
    //解析json字符串，res需转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    //json解析错误
    if (jsonDoc.isNull()){
        showTip(tr("json解析错误"), false);
        return;
    }
    
    //json解析错误，如果jsonDoc包含json对象，isObject返回true
    if (!jsonDoc.isObject()){
        showTip(tr("json解析错误"), false);
        return;
    }
    
    //调用对应逻辑
    return;
}
~~~

### 注册消息处理

在RegisterDialog中声明注册消息处理

~~~c++
void initHttpHandlers();
QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
~~~

实现initHttpHandlers

~~~c++
void RegisterDialog::initHttpHandlers()
{
    //注册获取验证码回包逻辑
    _handlers.insert(ReqId::ID_GET_VERIFY_CODE, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已发到邮箱，注意查收"), true);
        qDebug() << "email is " << email;
    });
}
~~~

回到slot_reg_mod_finish函数中，根据id回调对应逻辑

~~~c++
void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    //根据id回调对应逻辑
    _handlers[id](jsonDoc.object());

    return;
}
~~~



