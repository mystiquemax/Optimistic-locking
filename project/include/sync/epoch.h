#pragma once

#include <atomic>
#include <memory>
#include <vector>

namespace FinalProject {

/**
 * TODO: Assignment 2
 * Epoch memory management
 *
 * Paper: https://www.cs.cit.tum.de/fileadmin/w00cfj/dis/papers/reclaim.pdf
 */
struct EpochHandler {
  static constexpr uint64_t MAX_VALUE            = ~0ULL;
  static constexpr uint64_t MAX_NUMBER_OF_WORKER = 128;

  struct ToFreePointer {
    void *ptr;
    uint64_t epoch;
  };

  explicit EpochHandler(uint64_t no_threads);
  ~EpochHandler();

  void FreeOutdatedPtr(uint64_t tid);
  void AdvanceGlobalEpoch();
  void DeferFreePointer(uint64_t tid, void *ptr);

  /* Epoch-based ptr reclaimation */
  const uint64_t no_threads;
  std::atomic<uint64_t> global_epoch;
  std::vector<std::atomic<uint64_t>> local_epoch;
  // vector of to_free_ptr for each thread
  std::vector<std::vector<ToFreePointer>> to_free_ptr = {};
};

/**
 * RAII-styled struct to automatically manage local_epoch lifecycle of every thread
 */
class EpochGuard {
 public:
  EpochGuard(std::atomic<uint64_t> *local_epoch, const std::atomic<uint64_t> &global_epoch);
  ~EpochGuard();

 private:
  std::atomic<uint64_t> *epoch_;
};

}  // namespace FinalProject