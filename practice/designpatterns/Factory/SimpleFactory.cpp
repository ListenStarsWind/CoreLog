#include <iostream>
#include <memory>
#include <string>

using namespace std;

// 依赖倒置原则   先设计抽象类
class Fruit
{
   public:
    virtual void name() = 0;
};

class Apple : public Fruit
{
   public:
    void name() override { std::cout << "这是一个苹果\n"; }
};

class Banana : public Fruit
{
   public:
    void name() override { std::cout << "这是一个香蕉\n"; }
};

class FruitFactory
{
   public:
    // 使用抽象类指针进行适配
    static std::shared_ptr<Fruit> create(const string& id)
    {
        if (id == "苹果")
            return std::make_shared<Apple>();
        else if (id == "香蕉")
            return std::make_shared<Banana>();
        else
        {
            std::cout << "不存在的类型识别码" << std::endl;
            return {};                                          // 列表初始化返回一个空的默认构造智能指针
        }
    }
};

int main()
{
    std::shared_ptr<Fruit> fruit = FruitFactory::create("香蕉");
    fruit->name();
    fruit = FruitFactory::create("苹果");
    fruit->name();
    return 0;
}
