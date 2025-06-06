#include <iostream>
#include <memory>
#include <string>

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

class Animal
{
   public:
    virtual void name() = 0;
};

class Lamp : public Animal
{
   public:
    void name() override { std::cout << "这是一只山羊\n"; }
};

class Dog : public Animal
{
   public:
    void name() override { std::cout << "这是一只小狗\n"; }
};

class Factory
{
   public:
    virtual std::shared_ptr<Fruit> getFruit(const std::string& id) = 0;
    virtual std::shared_ptr<Animal> getAnimal(const std::string& id) = 0;
};

class FruitFactory : public Factory
{
   public:
    std::shared_ptr<Fruit> getFruit(const std::string& id) override
    {
        if (id == "苹果")
            return std::make_shared<Apple>();
        else if (id == "香蕉")
            return std::make_shared<Banana>();
        else
        {
            std::cout << "不存在的类型识别码" << std::endl;
            return {};
        }
    }
    std::shared_ptr<Animal> getAnimal(const std::string& id) override
    {
        std::cout << "被舍弃的接口\n";
        return {};
    }
};

class AnimalFactory : public Factory
{
   public:
    std::shared_ptr<Fruit> getFruit(const std::string& id) override
    {
        std::cout << "被舍弃的接口\n";
        return {};
    }
    std::shared_ptr<Animal> getAnimal(const std::string& id) override
    {
        if (id == "山羊")
            return std::make_shared<Lamp>();
        else if (id == "小狗")
            return std::make_shared<Dog>();
        else
        {
            std::cout << "不存在的类型识别码" << std::endl;
            return {};
        }
    }
};

// 水果和动物似乎不是同一级别上的抽象概念
// 但这里为了方便, 我们还是写一块
class FactoryProducer
{
   public:
    static std::shared_ptr<Factory> create(const std::string& kinds)
    {
        if (kinds == "水果")
        {
            return std::make_shared<FruitFactory>();
        }
        else if (kinds == "动物")
        {
            return std::make_shared<AnimalFactory>();
        }
        else
        {
            std::cout << "不存在的种类" << std::endl;
            return {};
        }
    }
};

int main()
{
    std::shared_ptr<Factory> fruit_factory = FactoryProducer::create("水果");
    std::shared_ptr<Fruit> apple = fruit_factory->getFruit("苹果");
    std::shared_ptr<Fruit> banana = fruit_factory->getFruit("香蕉");

    std::shared_ptr<Factory> animal_factory = FactoryProducer::create("动物");
    std::shared_ptr<Animal> lamp = animal_factory->getAnimal("山羊");
    std::shared_ptr<Animal> dog = animal_factory->getAnimal("小狗");

    apple->name();
    banana->name();
    lamp->name();
    dog->name();

    return 0;
}
