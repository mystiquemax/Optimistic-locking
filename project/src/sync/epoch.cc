#include "sync/epoch.h"

namespace FinalProject {

EpochHandler::EpochHandler(uint64_t no_threads)
    : no_threads(std::min(no_threads, MAX_NUMBER_OF_WORKER)),
      global_epoch(0),
      local_epoch(this->no_threads),
      to_free_ptr(this->no_threads) {}

/* Make sure to delete all remaining un-freed pointers */
EpochHandler::~EpochHandler() {
  /* Free existing to_free pointers */
  for (auto &ptr_list : to_free_ptr) {
    for (auto &[ptr, epoch] : ptr_list) { free(ptr); }
  }
}

/**
 * Atomic increase the global epoch
 */
void EpochHandler::AdvanceGlobalEpoch() { global_epoch.fetch_add(1); }

/**
 * Execute the epoch-based memory reclaimation
 *
 * 1. Take the minimum epoch of all threads - called "min_epoch"
 * 2. Go through all to-be-freed ptr of `tid` (to_free_ptr[tid]):
 *    Free all pointers whose epoch is < `min_epoch`
 */
void EpochHandler::FreeOutdatedPtr(uint64_t tid) {
  auto min_epoch = MAX_VALUE;
  for (const auto &epoch : local_epoch) { min_epoch = std::min(min_epoch, epoch.load()); }
  auto idx = 0ULL;
  for (; idx < to_free_ptr[tid].size(); idx++) {
    auto &[ptr, epoch] = to_free_ptr[tid][idx];
    if (epoch >= min_epoch) { break; }
    free(ptr);
  }
  to_free_ptr[tid].erase(to_free_ptr[tid].begin(), to_free_ptr[tid].begin() + idx);
}

/**
 * Append the ptr to `to_free_ptr[tid]`, set its usable epoch to
 * local_epoch[tid]
 */
void EpochHandler::DeferFreePointer(uint64_t tid, void *ptr) {
  to_free_ptr[tid].emplace_back(ptr, local_epoch[tid].load());
}

/**
 * - Set `epoch_` to the `local_epoch` of current thread id
 * - Update the `local_epoch` value to `global_epoch`
 */
EpochGuard::EpochGuard(std::atomic<uint64_t> *local_epoch, const std::atomic<uint64_t> &global_epoch)
    : epoch_(local_epoch) {
  epoch_->store(global_epoch.load());
}

/**
 * Set `epoch_` to `EpochHandler::MAX_VALUE`
 */
EpochGuard::~EpochGuard() { epoch_->store(EpochHandler::MAX_VALUE); }

}  // namespace FinalProject
