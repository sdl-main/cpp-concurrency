#include <thread>
#include <mutex>
#include <shared_mutex>
#include <iostream>
#include <vector>
#include <sstream>
#include <memory>

static int counter = 0;

template<typename T>
class TVector {
public:
    TVector() = default;
    TVector(const TVector& other) {
        std::scoped_lock(other.m);
        data = other.data;
    }
    TVector& operator =(const TVector& other) = delete;

    void add(const T& x) {
        std::scoped_lock lock{m};
        data.push_back(x);
    }

    std::string print() const {
        std::stringstream res;
        std::shared_lock lock(m);
        for (int p : data) {
            res << p << " ";
        }
        return res.str();
    }

private:
    std::vector<T> data;
    mutable std::shared_mutex m;
};

class TAdder {
private:
    std::shared_ptr<TVector<int>> vec;
    int cnt;

public:
    explicit TAdder(std::shared_ptr<TVector<int>> vec) : vec(std::move(vec)), cnt(0) {}
    void add() {
        for (int i = 0; i < 10; ++i) {
            vec->add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 100));
        }
    }
};

class TReader {
private:
    std::shared_ptr<TVector<int>> vec;

public:
    explicit TReader(std::shared_ptr<TVector<int>> vec) : vec(std::move(vec)) {}
    void read(const int delay) {
        for (int i = 0; i < 10; ++i) {
            std::cout << vec->print() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
    }
};

int main() {
    std::shared_ptr<TVector<int>> vec = std::make_shared<TVector<int>>();
    std::vector<TAdder> adders;
    for (int i = 0; i < 4; ++i) {
        adders.emplace_back(vec);
    }
    TReader reader(vec);
    std::vector<std::thread> pool;
    for (auto& adder : adders) {
        pool.emplace_back(&TAdder::add, &adder);
    }
    pool.emplace_back(&TReader::read, &reader, 500);
    pool.emplace_back(&TReader::read, &reader, 550);
    for (auto& t : pool) {
        t.join();
    }
    return 0;
}
