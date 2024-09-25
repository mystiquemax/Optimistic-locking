#pragma once

#include "sync/lock.h"

namespace FinalProject {

/**
 * Four possible state for HybridGuard::GuardMode
 * - OPTIMISTIC, SHARED, and EXCLUSIVE: the three locking mode for the optimistic lock
 * - MOVED: After Unlock()/ValidateOptimisticLock()
 */
enum class GuardMode { OPTIMISTIC, SHARED, EXCLUSIVE, MOVED };

/**
 * TODO: Assignment 2
 * A RAII-styled struct to manage the HybridLock.
 * During construction, it should store the lock state + version counter of HybridLock into `state_`
 *
 * Additional tasks:
 * - Optimistic lock:
 *  + Construction:  If the hybrid lock is held in Exclusive mode,
 *      sleep until it is backed to either Unlocked or Shared
 *  + Destruction: validate the lock state + version counter using the stored `state_`
 * - Shared lock:
 *  + Construction: Acquire the lock in shared mode
 *
 * After calling "Unlock", make sure to set the mode_ = GuardMode::MOVED to prevent double unlocked
 */
class HybridGuard {
 public:
  HybridGuard(HybridLock *lock, GuardMode mode);
  auto operator=(HybridGuard &&other) noexcept(false) -> HybridGuard &;
  ~HybridGuard() noexcept(false);

  /* Work for both SHARED and EXCLUSIVE */
  void Unlock();

  /**
   * Two APIs for the optimistic lock lifecycle
   *
   * Description:
   * - OptimisticLock(): Sleeping until the HybridLock is not in EXCLUSIVE mode
   *   After that, store a copy of the lock_->StateAndVersion().load() in state_
   * - ValidateOptimisticLock(): Compare the local copy with the latest lock_->StateAndVersion().load().
   *   If the LockState(latest) is EXCLUSIVE or Version(latest) != Version(this->state_), throw RestartException()
   *
   * On the caller, place the HybridGuard inside a try-catch group.
   * See test.cc for your information
   */
  void OptimisticLock();
  void ValidateOptimisticLock();

 private:
  HybridLock *lock_;
  GuardMode mode_;
  uint64_t state_{0};
};

}  // namespace FinalProject