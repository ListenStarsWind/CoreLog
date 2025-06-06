#include<iostream>

class RentHouse{
    public:
    virtual void rentHouse() = 0;
};

class Landlord : public RentHouse{
    public:
    void rentHouse() override {
        std::cout<<"将房子租出去\n";
    }
};

class Intermediary : public RentHouse{
    public:
    void rentHouse() override{
        std::cout<<"发布招租启示\n";
        std::cout<<"带人看房\n";
        _landlord.rentHouse();
        std::cout<<"负责租后维修\n";
    }

    private:
    Landlord _landlord;
};

int main()
{
    Intermediary intermediary;
    intermediary.rentHouse();
    return 0;
}
