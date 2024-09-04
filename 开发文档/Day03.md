#### 增加singleton模板单例类

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