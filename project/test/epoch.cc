#include <semaphore.h>
#include <array>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "common/utest.h"
#include "common/utils.h"
#include "sync/epoch.h"
#include "sync/guard.h"

static constexpr int NO_THREADS = 10;
static constexpr int NO_ENTRIES = 10000;

using HybridLock       = FinalProject::HybridLock;
using HybridGuard      = FinalProject::HybridGuard;
using GuardMode        = FinalProject::GuardMode;
using EpochHandler     = FinalProject::EpochHandler;
using EpochGuard       = FinalProject::EpochGuard;
using RestartException = FinalProject::RestartException;

UTEST(TestGuard, OptimisticValidationThrow) {
  HybridLock latch;
  try {
    HybridGuard op_guard(&latch, GuardMode::OPTIMISTIC);
    { HybridGuard guard(&latch, GuardMode::EXCLUSIVE); }
    ASSERT_EXCEPTION(op_guard.ValidateOptimisticLock(), FinalProject::RestartException);
  } catch (const FinalProject::RestartException &) {}
}

UTEST(TestGuard, NormalOperation) {
  int counter                             = 0;
  std::atomic<int> optimistic_restart_cnt = 0;
  HybridLock latch;

  std::thread threads[NO_THREADS];

  for (int idx = 0; idx < NO_THREADS; idx++) {
    // 50% Write, 50% Read
    if (idx % 2 == 0) {
      threads[idx] = std::thread([&]() {
        HybridGuard guard(&latch, GuardMode::EXCLUSIVE);
        counter++;
      });
    } else {
      threads[idx] = std::thread([&]() {
        while (true) {
          auto guard_mode = (rand() % 10 == 0) ? GuardMode::SHARED : GuardMode::OPTIMISTIC;
          try {
            HybridGuard guard(&latch, guard_mode);
            EXPECT_TRUE((0 <= counter) && (counter <= NO_THREADS / 2));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            try {
              // In reality, we don't need to trigger the optimistic check like this
              //   as it is automatically triggered during guard destruction
              guard.ValidateOptimisticLock();
              break;
            } catch (const RestartException &) { optimistic_restart_cnt++; }
          } catch (const RestartException &) {}
        }
      });
    }
  }

  for (auto &thread : threads) { thread.join(); }

  EXPECT_EQ(counter, NO_THREADS / 2);
  EXPECT_GT(optimistic_restart_cnt.load(), 0);
}

// Duy: For MacOS students: Semaphore was deprecated, thus this test will not run properly in your machine
// Two possible solutions:
// - Run this in a Linux machine
// - Replace sem_t with std::binary_semaphore
// Whoever can do the 2nd will receive a bonus - make sure to send me an email regarding this
UTEST(TestEpoch, NormalOperation) {
  EpochHandler man(NO_THREADS);

  std::vector<std::array<void *, NO_ENTRIES + 1>> ptrs(NO_THREADS);
  for (auto idx = 0; idx < NO_THREADS; idx++) {
    for (auto epoch = 1; epoch <= NO_ENTRIES; epoch++) { ptrs[idx][epoch] = malloc(128); }
  }

  std::thread threads[NO_THREADS];
  std::array<std::unique_ptr<std::binary_semaphore>, NO_THREADS> signal_main_to_thread;
  std::array<std::unique_ptr<std::binary_semaphore>, NO_THREADS> signal_thread_to_main;

  for (int tid = 0; tid < NO_THREADS; tid++) {
    signal_main_to_thread[tid] = std::make_unique<std::binary_semaphore>(0);
    signal_thread_to_main[tid] = std::make_unique<std::binary_semaphore>(0);
  }

  /* Worker threads */
  for (int tid = 0; tid < NO_THREADS; tid++) {
    threads[tid] = std::thread([&, thread_id = tid]() {
      for (auto loop_epoch = 1; loop_epoch <= NO_ENTRIES; loop_epoch++) {
        signal_main_to_thread[thread_id]->acquire();

        /* Delete all local ptrs < min epochs */
        man.FreeOutdatedPtr(thread_id);

        {
          EpochGuard ep(&(man.local_epoch[thread_id]), man.global_epoch);

          /* All ptrs of current epoch should be fine to read */
          for (auto t_id = 0; t_id < NO_THREADS; t_id++) {
            auto ptr = ptrs[t_id][loop_epoch];
            auto x   = reinterpret_cast<int>(*(reinterpret_cast<int *>(ptr)));
          }

          /* Delete current ptr */
          man.DeferFreePointer(thread_id, ptrs[thread_id][loop_epoch]);
        }

        /* One worker advances the global epoch */
        if (thread_id == 0) { man.AdvanceGlobalEpoch(); }

        /* Notify main thread this thread completes current epoch */
        signal_thread_to_main[thread_id]->release();
        std::atomic_thread_fence(std::memory_order_seq_cst);
      }
    });
  }

  /* Main thread */
  for (auto loop_epoch = 1; loop_epoch <= NO_ENTRIES; loop_epoch++) {
    /* Signal all workers to execute next epoch */
    for (int tid = 0; tid < NO_THREADS; tid++) { signal_main_to_thread[tid]->release(); }

    /* Wait until all workers complete this epoch */
    for (int tid = 0; tid < NO_THREADS; tid++) { signal_thread_to_main[tid]->acquire(); }

    /* Every defer list should have not more than two pointers */
    std::atomic_thread_fence(std::memory_order_seq_cst);
    for (auto &ptr_list : man.to_free_ptr) { ASSERT_TRUE(ptr_list.size() <= 2); }
  }

  for (auto &thread : threads) { thread.join(); }

  /* AdvanceGlobalEpoch() was called NO_ENTRIES times */
  ASSERT_EQ(man.global_epoch.load(), NO_ENTRIES);
  for (auto &ptr_list : man.to_free_ptr) { ASSERT_TRUE(ptr_list.size() <= 2); }

  /* Epoch handler destructor should automatically reclaim all un-deleted ptrs */
}

UTEST_MAIN();
