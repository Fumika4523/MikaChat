#ifndef GLOBAL_H
#define GLOBAL_H
#include <QWidget>
#include <functional>
#include "QStyle"
#include <QRegularExpression>
#include <memory>
#include <iostream>
#include <mutex>

/**
 * @brief repolish 用来刷新qss
 */
extern std::function<void(QWidget*)> repolish;
//请求id
enum ReqId{
    ID_GET_VERIFY_CODE = 1001,
    ID_REG_USER = 1002,
};
//错误代码
enum ErrorCodes{
    SUCCESS = 0,
    ERR_JSON = 1,
    ERR_NETWORK = 2,
};
//模块
enum Modules{
    REGISTERMOD = 0,
};

#endif // GLOBAL_H
