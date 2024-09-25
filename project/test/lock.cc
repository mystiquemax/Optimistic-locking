#include "sync/lock.h"
#include "common/utest.h"

#include <chrono>
#include <thread>

static constexpr int NO_THREADS = 100;
static constexpr int NO_OPS     = 1000;

using namespace FinalProject;

UTEST(TestHybridLock, SerializeOperation) {
  HybridLock lock;

  // Exclusive Latch then all subsequent latch attempts should fail
  EXPECT_TRUE(lock.TryLockExclusive(lock.StateAndVersion().load()));
  for (auto idx = 0; idx < 5; idx++) { EXPECT_FALSE(lock.TryLockShared(lock.StateAndVersion())); }
  EXPECT_FALSE(lock.TryLockExclusive(lock.StateAndVersion()));
  lock.UnlockExclusive();

  // Shared Latch, then only 254 later shared latch attemps should success
  EXPECT_TRUE(lock.TryLockShared(lock.StateAndVersion()));
  for (size_t idx = 1; idx < HybridLock::MAX_SHARED; idx++) { EXPECT_TRUE(lock.TryLockShared(lock.StateAndVersion())); }
  EXPECT_FALSE(lock.TryLockShared(lock.StateAndVersion()));
  EXPECT_FALSE(lock.TryLockExclusive(lock.StateAndVersion()));
  for (size_t idx = 0; idx < HybridLock::MAX_SHARED; idx++) { lock.UnlockShared(); }
}

UTEST(TestHybridLock, DowngradeLock) {
  HybridLock lock;
  EXPECT_TRUE(lock.TryLockExclusive(lock.StateAndVersion().load()));
  EXPECT_FALSE(lock.TryLockShared(lock.StateAndVersion()));
  lock.DowngradeLock();
  for (size_t idx = 1; idx < HybridLock::MAX_SHARED; idx++) { EXPECT_TRUE(lock.TryLockShared(lock.StateAndVersion())); }
  EXPECT_FALSE(lock.TryLockShared(lock.StateAndVersion()));
}

UTEST(TestHybridLock, UpgradeLock) {
  HybridLock lock;
  EXPECT_TRUE(lock.TryLockShared(lock.StateAndVersion()));
  EXPECT_FALSE(lock.TryLockExclusive(lock.StateAndVersion()));
  EXPECT_TRUE(lock.UpgradeLock(lock.StateAndVersion()));
  EXPECT_FALSE(lock.TryLockExclusive(lock.StateAndVersion()));
}

UTEST(TestHybridLock, NormalOperation) {
  int counter = 0;
  HybridLock lock;

  std::thread threads[NO_THREADS];
  std::atomic_bool is_running = true;

  for (int idx = 0; idx < NO_THREADS; idx++) {
    // 50% Write, 50% Read
    if (idx % 2 == 0) {
      threads[idx] = std::thread([&]() {
        auto flag = false;
        while (!flag && is_running.load()) {
          auto old_stat_w = lock.StateAndVersion().load();
          flag            = lock.TryLockExclusive(old_stat_w);
        }
        counter++;
        lock.UnlockExclusive();
      });
    } else {
      threads[idx] = std::thread([&]() {
        auto flag = false;
        while (!flag && is_running.load()) {
          auto old_stat_w = lock.StateAndVersion().load();
          flag            = lock.TryLockShared(old_stat_w);
        }
        EXPECT_TRUE((0 <= counter) && (counter <= NO_THREADS / 2));
        lock.UnlockShared();
      });
    }
  }

  std::this_thread::sleep_for(std::chrono::seconds(1));
  is_running = false;
  for (auto &thread : threads) { thread.join(); }

  EXPECT_EQ(counter, NO_THREADS / 2);
}

UTEST(TestHybridLock, HeavyOperation) {
  int counter = 0;
  HybridLock lock;

  std::thread threads[NO_THREADS];
  std::atomic_bool is_running = true;

  for (int idx = 0; idx < NO_THREADS; idx++) {
    // 50% Write, 50% Read
    if (idx % 2 == 0) {
      threads[idx] = std::thread([&]() {
        for (auto idx = 0; (idx < NO_OPS) && (is_running.load()); idx++) {
          auto flag = lock.TryLockExclusive(lock.StateAndVersion().load());
          while (!flag && is_running.load()) {
            std::this_thread::yield();
            auto old_stat_w = lock.StateAndVersion().load();
            flag            = lock.TryLockExclusive(old_stat_w);
          }
          counter++;
          lock.UnlockExclusive();
        }
      });
    } else {
      threads[idx] = std::thread([&]() {
        for (auto idx = 0; (idx < NO_OPS) && (is_running.load()); idx++) {
          auto flag = lock.TryLockShared(lock.StateAndVersion().load());
          while (!flag && is_running.load()) {
            std::this_thread::yield();
            auto old_stat_w = lock.StateAndVersion().load();
            flag            = lock.TryLockShared(old_stat_w);
          }
          EXPECT_TRUE((0 <= counter) && (counter <= NO_THREADS * NO_OPS / 2));
          lock.UnlockShared();
        }
      });
    }
  }

  std::this_thread::sleep_for(std::chrono::seconds(1));
  is_running = false;
  for (auto &thread : threads) { thread.join(); }

  EXPECT_EQ(counter, NO_THREADS * NO_OPS / 2);
}

UTEST_MAIN();
