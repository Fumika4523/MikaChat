### qss样式切换

在registerdialog增加err_tip![image-20240903184929043](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20240903184929043.png)

stylesheet中增加样式

~~~ 
#err_tip[state='normal']{
    color:green;
}

#err_tip[state='err']{
    color:red;
}
~~~

增加全局文件global，写入刷新qss函数，如下

~~~
/**
 * @brief repolish 用来刷新qss
 */
extern std::function<void(QWidget*)> repolish;

~~~

~~~
std::function<void(QWidget*)> repolish = [](QWidget* w){
    w->style()->unpolish(w);
    w->style()->polish(w);
};
~~~

registerdialog中设置normal属性，调用repolish，刷新qss

~~~
ui->err_tip->setProperty("state","normal");
repolish(ui->err_tip);
~~~

编写get_code槽函数，匹配email_edit，判断邮箱地址是否正确

~~~C
void RegisterDialog::on_get_code_clicked()
{
    auto email = ui->email_edit->text();
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch();
    if (match){
        //发送http验证码
    }else{
        showTip(tr("邮箱地址不正确"), false);
    }
}
~~~

编写showTip函数，当邮箱错误时提示错误信息，刷新qss

~~~C
void RegisterDialog::showTip(QString str, bool b_ok)
{
    if (b_ok){
        ui->err_tip->setProperty("state","normal");
    }else{
        ui->err_tip->setProperty("state","err");
    }
    ui->err_tip->setText(str);
    repolish(ui->err_tip);
}
~~~

