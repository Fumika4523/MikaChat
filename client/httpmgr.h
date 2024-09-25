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
#include <QNetworkRequest>
#include <QNetworkReply>

class HttpMgr:public QObject, public Singleton<HttpMgr>,
        public std::enable_shared_from_this<HttpMgr>
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
    void sig_http_finish(ReqId, QString res, ErrorCodes err, Modules mod);
};

#endif // HTTPMGR_H
