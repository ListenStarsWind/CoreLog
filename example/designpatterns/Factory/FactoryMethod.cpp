#include <iostream>
#include <memory>

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
    virtual std::shared_ptr<Fruit> create() = 0;
};

class AppleFactory : public FruitFactory
{
   public:
    std::shared_ptr<Fruit> create() override { return std::make_shared<Apple>(); }
};

class BananaFactory : public FruitFactory
{
   public:
    std::shared_ptr<Fruit> create() override { return std::make_shared<Banana>(); }
};

int main()
{
    std::shared_ptr<FruitFactory> factory(new AppleFactory());
    std::shared_ptr<Fruit> fruit = factory->create();
    fruit->name();

    factory.reset(new BananaFactory());
    fruit = factory->create();
    fruit->name();

    return 0;
}
