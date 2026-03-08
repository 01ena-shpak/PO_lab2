#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

std::vector<int> GenerateArray(int n)
{
    std::vector<int> a(n);

    for (int i = 0; i < n; i++) {
        a[i] = std::rand() % 1000 - 500;
    }

    return a;
}

void PrintArray(const std::vector<int>& a)
{
    for (int i = 0; i < (int)a.size(); i++) {
        std::cout << a[i] << " ";
    }
    std::cout << "\n";
}

struct Result
{
    int count;
    int minValue;
    bool found;
};

Result FindSequential(const std::vector<int>& a)
{
    Result r;

    r.count = 0;
    r.minValue = 0;
    r.found = false;

    for (int i = 0; i < (int)a.size(); i++) {

        if (a[i] % 21 == 0) {

            r.count++;

            if (!r.found || a[i] < r.minValue) {
                r.minValue = a[i];
                r.found = true;
            }
        }
    }

    return r;
}

void PrintResult(const Result& r)
{
    std::cout << "count = " << r.count << "\n";

    if (r.found)
        std::cout << "min = " << r.minValue << "\n";
    else
        std::cout << "min not found\n";
}

int main()
{
    std::srand(std::time(nullptr));

    int N = 10;

    auto a = GenerateArray(N);

    std::cout << "Array:\n";
    PrintArray(a);

    auto result = FindSequential(a);

    std::cout << "\nSequential result:\n";
    PrintResult(result);

    return 0;
}
