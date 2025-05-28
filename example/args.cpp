#include <iostream>

using namespace std;

void xprintf()
{
    cout << endl;
}

template <typename T, typename... Args>
void xprintf(const T& val, Args&&... args)
{
    cout << val << " ";
    xprintf(std::forward<Args>(args)...);
}

int main()
{
    xprintf(1, 2.3, 5, 7.04, "abcdef");
    xprintf(6, 5, 3.14);
    return 0;
}