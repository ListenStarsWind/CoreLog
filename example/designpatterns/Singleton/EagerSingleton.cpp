#include <iostream>

using namespace std;

class Singleton
{
   private:
    Singleton() : _data(80) { cout << "单例实例化" << endl; }
    ~Singleton() {}

    // 禁止以拷贝方式再实例化一个对象
    Singleton(const Singleton&) = delete;

   public:
    // 静态成员函数提供全局访问点
    static Singleton& getInstance() { return _eton; }

    int getData() { return _data; }

   private:
    int _data;

    // 类内声明一个自身的静态变量
    static Singleton _eton;
};

Singleton Singleton::_eton;

int main()
{
    cout << "main函数开始运行"<<endl;
    cout << Singleton::getInstance().getData()<<endl;
    return 0;
}
