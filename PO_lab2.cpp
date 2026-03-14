#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <climits>

std::vector<int> GenerateArray(int n)
{
    std::vector<int> a(n);

    for (int i = 0; i < n; i++) {
        a[i] = std::rand() % 10000 - 5000;
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

double TimeSequentialMs(const std::vector<int>& a, Result& out)
{
    auto t1 = std::chrono::high_resolution_clock::now();

    out = FindSequential(a);

    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(t2 - t1);
    return ms.count();
}

double TimeMutexMs(const std::vector<int>& a, int threads, Result& out)
{
    auto t1 = std::chrono::high_resolution_clock::now();

    out = FindParallelMutex(a, threads);

    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(t2 - t1);
    return ms.count();
}

double TimeAtomicMs(const std::vector<int>& a, int threads, Result& out)
{
    auto t1 = std::chrono::high_resolution_clock::now();

    out = FindParallelAtomicCAS(a, threads);

    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(t2 - t1);
    return ms.count();
}

int main()
{
    std::srand(std::time(nullptr));

    int THREADS = 4;

    std::cout << "Physical cores = 10, Logical processors = 16\n\n";

    int sizes[] = { 100000, 500000, 1000000, 5000000, 10000000 };

    for (int si = 0; si < 5; si++) {

        int N = sizes[si];

        auto base = GenerateArray(N);

        std::cout << "N = " << N << "\n";

        Result rSeq;
        double tSeq = TimeSequentialMs(base, rSeq);

        std::cout << "normal_ms = " << tSeq << "\n";
        std::cout << "normal_count = " << rSeq.count << " normal_min = " << rSeq.minValue << " found = " << rSeq.found << "\n";

        std::cout << "threads = " << THREADS << "\n";

        Result rMutex;
        Result rAtomic;

        double tMutex = TimeMutexMs(base, THREADS, rMutex);
        double tAtomic = TimeAtomicMs(base, THREADS, rAtomic);

        std::cout << "mutex_ms = " << tMutex << "\n";
        std::cout << "mutex_count = " << rMutex.count << " mutex_min = " << rMutex.minValue << " found = " << rMutex.found << "\n";

        std::cout << "atomic_ms = " << tAtomic << "\n";
        std::cout << "atomic_count = " << rAtomic.count << " atomic_min = " << rAtomic.minValue << " found = " << rAtomic.found << "\n";

        std::cout << "-----------------------\n";
    }

    return 0;
}
