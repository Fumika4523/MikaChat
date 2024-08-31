### 创建应用

用qt创建at widgets application

![image-20240901004632257](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20240901004632257.png)

选择默认选项，生成项目

![image-20240901004752091](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20240901004752091.png)

为了增加项目可读性，增加注释模板

选择"工具"->"外部"->"配置"，再次选择"文本编辑器"->"片段"->"添加"，按照下面的模板编排

```
/******************************************************************************
 *
 * @file       %{CurrentDocument:FileName}
 * @brief      XXXX Function
 *
 * @author     袁文伽
 * @date       %{CurrentDate:yyyy\/MM\/dd}
 * @history    
 *****************************************************************************/
```

如下图

![image-20240901005325764](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20240901005325764.png)

以后输入header_custom就可以弹出注释模板了

修改mainwindow.ui![image-20240901010540609](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20240901010540609.png)

window title改为MikaChat

将icon.ico文件放到根目录，然后在项目pro里添加输出目录文件和ico图标

~~~
RC_ICONS = icon.ico
DESTDIR = ./bin
~~~

将图片资源添加ice.png添加到文件夹res里，然后右键项目选择添加新文件，选择qt resource files， 添加qt的资源文件，名字设置为rc。

添加成功后邮件rc.qrc选择添加现有资源文件，

选择res文件夹下的ice.png，这样ice.png就导入项目工程了。

### 创建登录界面

右键项目，选择添加新文件，点击Qt Widgets Designer Form Class

![image-20240901011323316](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20240901011323316.png)

选择 dialog without buttons

![image-20240901011339961](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20240901011339961.png)

类名叫做LoginDialog

![image-20240901011409896](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20240901011409896.png)

将LoginDialog.ui修改为以下布局

![image-20240901011603225](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20240901011603225.png)

在mainwindows.h添加LoginWidget指针成员，然后在构造函数将LoginWidget设置为中心部件

```c++
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog();
    setCentralWidget(_login_dlg);
    _login_dlg->show();
}
```

### 创建注册界面

与登录界面类似，创建的界面如下：

![image-20240901012101888](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20240901012101888.png)

创建好界面后，在LoginDialog类声明里添加信号切换注册界面

~~~c++
signals:
    void switchRegister();
~~~

在LoginDialog构造函数里连接点击事件

~~~C
connect(ui->reg_button, &QPushButton::clicked, this, &LoginDialog::switchRegister);		
~~~

按钮点击后，LoginDialog发送switchRegister信号，该信号发送给MainWindow用来切换界面。

在MainWindow里声明RigsterDialog指针

```c
private:
    RegisterDialog* _reg_dlg;
```

在其构造函数中添加RigsterDialog类的初始化，以及连接switchRegister信号

```c
    //创建和注册消息的链接
    connect(_login_dlg, &LoginDialog::switchRegister,
            this, &MainWindow::SlotSwitchReg);

    _reg_dlg = new RegisterDialog();
```

接下来实现槽函数SlotSwitchReg

```c
void MainWindow::SlotSwitchReg(){
    setCentralWidget(_reg_dlg);
    _login_dlg->hide();
    _reg_dlg->show();
}
```

这样启动程序主界面优先显示登录界面，点击注册后跳转到注册界面

### 优化样式

删除mainwindow.ui里的toolbar，在项目根目录下创建style文件夹，在文件夹里创建stylesheet.qss文件，然后在qt项目中的rc.qrc右键添加现有文件，选择stylesheet.qss，这样qss就被导入到项目中了。

在主程序启动后加载qss

```
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile qss(":/style/stylesheet.qss");

    if( qss.open(QFile::ReadOnly))
    {
        qDebug("open success");
        QString style = QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    }else{
         qDebug("Open failed");
     }

    MainWindow w;
    w.show();

    return a.exec();
}
```

然后写qss样式美化界面

```
QDialog#LoginDialog{
background-color:rgb(255,255,255)
}
```