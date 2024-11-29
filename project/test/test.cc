#include "common/utest.h"
#include "list/list.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>
#include <algorithm>
#include "PerfEvent.hpp"
#include <chrono>
#include <cstdint>
#include <cstddef> 
#include <string>
#include <random>

static constexpr int NO_THREADS = 128;
static constexpr int NO_OPS     = 1000;

using namespace FinalProject;

struct alignas(32) Student {
private:
    unsigned matrID; // 4
    std::string name; // 20
    uint8_t currentSemester; // 1 => 28 bytes with alignment

public:
    Student() : matrID(0), name(std::to_string(0)), currentSemester(0){}

    Student(int matrID, const std::string& name, uint8_t currentSemester)
        : matrID(matrID), name(name), currentSemester(currentSemester) {}

    auto operator<=>(const Student& other) const {
        return matrID <=> other.matrID;
    }
};

auto Generate(unsigned n) -> std::vector<unsigned> {
  std::vector<unsigned> data(n);
  std::iota(data.begin(), data.end(), 0);
  std::shuffle(data.begin(), data.end(), std::mt19937(42));
  return data;
}
int getRandomNumber(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

UTEST(TestMutexSortedList, LookUp){
 // Single threaded insert, multithreaded look up. Only track the lookup 
  MutexSortedList<Student> list;
  
  std::thread threads[NO_THREADS];

  auto data = Generate(NO_OPS);
  for (auto key : data) { 
   Student student = Student(key, std::to_string(key), getRandomNumber(1,8));
    list.Insert(student); 
  }
  
  PerfEvent e;
  e.startCounters();
  for (auto idx = 0; idx < NO_THREADS; idx++) {
    threads[idx] = std::thread([&, tid = idx]() {
      Student value;
      for(int i = 0; i < tid; i++){
        Student student = Student(tid, std::to_string(tid), getRandomNumber(1,8));
         list.LookUp(student, value);
      }
    });
  }

  for (auto &thread : threads) { thread.join(); }
  e.stopCounters();
  e.printReport(std::cout, NO_THREADS);
}

UTEST(TestOptimisticSortedList, LookUp){
  // Single threaded insert, multithreaded look up. Only track the lookup
  EpochHandler epoch(NO_THREADS);
  OptimisticSortedList<Student> list(&epoch);
  std::thread threads[NO_THREADS];

  auto data = Generate(NO_OPS);
  for (auto key : data) { 
    Student student = Student(key, std::to_string(key), getRandomNumber(1,8));
    list.Insert(student); 
  }
  
  PerfEvent e;
  e.startCounters();
  for (auto idx = 0; idx < NO_THREADS; idx++) {
    threads[idx] = std::thread([&, tid = idx]() {
      InitializeThread();
      Student value;
      for(int i = 0; i < tid; i++){
         Student student = Student(tid, std::to_string(tid), getRandomNumber(1,8));
         list.LookUp(student, value);
      }
    });
  }

  for (auto &thread : threads) { thread.join(); }
  e.stopCounters();
  e.printReport(std::cout, NO_THREADS);
}

UTEST_MAIN();
