#include "common/utest.h"
#include "list/list.h"

#include <chrono>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>

static constexpr int NO_THREADS = 10;
static constexpr int NO_OPS     = 1000;

using namespace FinalProject;

auto Generate(unsigned n) -> std::vector<unsigned> {
  std::vector<unsigned> data(n);
  iota(data.begin(), data.end(), 0);
  shuffle(data.begin(), data.end(), std::mt19937(42));
  return data;
}

UTEST(TestMutexSortedList, NormalOperation) {
  MutexSortedList list;
  auto data = Generate(NO_OPS);
  for (auto key : data) { list.Insert(key, key * 2); }
  for (auto idx = 0; idx < NO_OPS; idx++) {
    int value;
    ASSERT_TRUE(list.LookUp(idx, value));
    ASSERT_EQ(value, idx * 2);
    ASSERT_TRUE(list.Delete(idx));
    ASSERT_FALSE(list.LookUp(idx, value));
    ASSERT_FALSE(list.Delete(idx));
  }
}

UTEST(TestMutexSortedList, MultiThreadInsertion) {
  MutexSortedList list;

  std::thread threads[NO_THREADS];
  for (int idx = 0; idx < NO_THREADS; idx++) {
    threads[idx] = std::thread([&, tid = idx]() {
      for (auto item = 0; item < NO_OPS; item++) { list.Insert(tid * NO_OPS + item, item); }
    });
  }

  for (auto &thread : threads) { thread.join(); }
  for (auto idx = 0; idx < NO_OPS * NO_THREADS; idx++) {
    int value;
    ASSERT_TRUE(list.LookUp(idx, value));
    ASSERT_EQ(value, idx % NO_OPS);
  }
}

UTEST_MAIN();
