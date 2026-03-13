#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <mutex>
#include <atomic>
#include <climits>

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

void WorkerMutex(
    const std::vector<int>& a,
    int start,
    int end,
    int& globalCount,
    int& globalMin,
    bool& globalFound,
    std::mutex& mtx)
{
    for (int i = start; i < end; i++) {

        if (a[i] % 21 == 0) {

            mtx.lock();

            globalCount++;

            if (!globalFound || a[i] < globalMin) {
                globalMin = a[i];
                globalFound = true;
            }

            mtx.unlock();
        }
    }
}

Result FindParallelMutex(const std::vector<int>& a, int threads)
{
    int n = (int)a.size();

    int globalCount = 0;
    int globalMin = 0;
    bool globalFound = false;

    std::mutex mtx;
    std::vector<std::thread> t;

    int part = n / threads;
    int start = 0;

    for (int i = 0; i < threads; i++) {

        int end = (i == threads - 1) ? n : start + part;

        t.push_back(std::thread(
            WorkerMutex,
            std::cref(a),
            start,
            end,
            std::ref(globalCount),
            std::ref(globalMin),
            std::ref(globalFound),
            std::ref(mtx)
        ));

        start = end;
    }

    for (int i = 0; i < (int)t.size(); i++)
        t[i].join();

    Result r;
    r.count = globalCount;
    r.minValue = globalMin;
    r.found = globalFound;

    return r;
}

void WorkerAtomicCAS(
    const std::vector<int>& a,
    int start,
    int end,
    std::atomic<int>& globalCount,
    std::atomic<int>& globalMin)
{
    for (int i = start; i < end; i++) {

        if (a[i] % 21 == 0) {

            int oldCount;
            int newCount;

            do {
                oldCount = globalCount.load();
                newCount = oldCount + 1;
            } while (!globalCount.compare_exchange_weak(oldCount, newCount));

            int oldMin;

            do {
                oldMin = globalMin.load();

                if (a[i] >= oldMin)
                    break;

            } while (!globalMin.compare_exchange_weak(oldMin, a[i]));
        }
    }
}

Result FindParallelAtomicCAS(const std::vector<int>& a, int threads)
{
    int n = (int)a.size();

    std::atomic<int> globalCount(0);
    std::atomic<int> globalMin(INT_MAX);

    std::vector<std::thread> t;

    int part = n / threads;
    int start = 0;

    for (int i = 0; i < threads; i++) {

        int end = (i == threads - 1) ? n : start + part;

        t.push_back(std::thread(
            WorkerAtomicCAS,
            std::cref(a),
            start,
            end,
            std::ref(globalCount),
            std::ref(globalMin)
        ));

        start = end;
    }

    for (int i = 0; i < (int)t.size(); i++)
        t[i].join();

    Result r;
    r.count = globalCount.load();
    r.minValue = globalMin.load();
    r.found = (r.count > 0);

    return r;
}

int main()
{
    std::srand(std::time(nullptr));

    int N = 100000;
    int threads = 4;

    auto a = GenerateArray(N);

    //std::cout << "Array:\n";
    //PrintArray(a);

    auto r1 = FindSequential(a);

    std::cout << "\nSequential:\n";
    PrintResult(r1);

    auto r2 = FindParallelMutex(a, threads);

    std::cout << "\nMutex parallel:\n";
    PrintResult(r2);

    auto r3 = FindParallelAtomicCAS(a, threads);

    std::cout << "\nAtomic CAS\n";
    PrintResult(r3);

    return 0;
}
