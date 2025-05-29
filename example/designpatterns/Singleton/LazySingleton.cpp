#include<iostream>

class Singleton
{
    private:
    Singleton() :_data(80){std::cout<<"单例实例化"<<std::endl;}
    ~Singleton(){}

    Singleton(const Singleton&) = delete;

    public:
    static Singleton& getInstance()
    {
        static Singleton _eton;
        return _eton;
    }

    int getData()
    {
        return _data;
    }

    private:
    int _data;
};

int main()
{
    std::cout<<"程序开始"<<std::endl;
    std::cout<< Singleton::getInstance().getData()<<std::endl;
    return 0;
}
