#include <iostream>
#include <memory>
#include <string>

class Computer
{
   public:
    Computer() {}
    void setBoard(const std::string& board) { _board = board; }

    void setDisplay(const std::string& display) { _display = display; }

    virtual void setOs() = 0;

    void showParamaters()
    {
        std::string param = "Computer paramaters:\n";
        param += "\tBoard: " + _board + '\n';
        param += "\tDisplay: " + _display + '\n';
        param += "\tOs: " + _os + '\n';
        std::cout << param << std::endl;
    }

   protected:
    // 构造电脑所需要的各种组件
    std::string _board;
    std::string _display;
    std::string _os;
};

class MacBook : public Computer
{
   public:
    void setOs() override { _os = "Mac OS x12"; }
};

class Builder
{
   public:
    virtual void buildBoard(const std::string& board) = 0;
    virtual void buildDisplay(const std::string& display) = 0;
    virtual void buildOs() = 0;
    virtual std::shared_ptr<Computer> build() = 0;
};

class MacBookBuilder : public Builder
{
   public:
    MacBookBuilder() : _computer(new MacBook()) {}
    void buildBoard(const std::string& board) override { _computer->setBoard(board); }
    void buildDisplay(const std::string& display) override { _computer->setDisplay(display); }
    void buildOs() override { _computer->setOs(); }
    std::shared_ptr<Computer> build() override { return _computer; }

   private:
    std::shared_ptr<Computer> _computer;
};

class Director
{
   public:
    Director(Builder* builder) : _builder(builder) {}
    void construct(const std::string& board, const std::string& display)
    {
        _builder->buildBoard(board);
        _builder->buildDisplay(display);
        _builder->buildOs();
    }

   private:
    std::shared_ptr<Builder> _builder;
};

int main()
{
    Builder* builder = new MacBookBuilder();
    std::unique_ptr<Director> director(new Director(builder));
    director->construct("富士康", "三星");
    std::shared_ptr<Computer> mac = builder->build();
    mac->showParamaters();
    return 0;
}
