#include "sync/guard.h"
#include "common/utils.h"

#include <stdexcept>
#include <thread>

namespace FinalProject {

/**
 * Depends on the provided GuardMode, acquire the lock
 *
 * - GuardMode::OPTIMISTIC: Call OptimisticLock()
 * - GuardMode::SHARED/EXCLUSIVE: Acquire Shared/Exclusive on `lock`. Sleep if necessary
 * - GuardMode::MOVED: Prevent this
 */
HybridGuard::HybridGuard(HybridLock *lock, GuardMode mode) : lock_(lock), mode_(mode) {
  switch (mode) {
    case GuardMode::OPTIMISTIC: OptimisticLock(); break;
    case GuardMode::SHARED: {
      uint64_t old_state;
      for (auto counter = 0;; counter++) {
        old_state = lock->StateAndVersion();
        if (lock->TryLockShared(old_state)) { break; }
        std::this_thread::yield();
      }
    } break;
    case GuardMode::EXCLUSIVE: {
      uint64_t old_state;
      for (auto counter = 0;; counter++) {
        old_state = lock->StateAndVersion();
        if (lock->TryLockExclusive(old_state)) { break; }
        std::this_thread::yield();
      }
    } break;
    default: throw std::logic_error("Please no");
  }
}

/**
 * COPY operator.
 * Make sure to validate the Optimistic State before the actual copy
 */
auto HybridGuard::operator=(HybridGuard &&other) noexcept(false) -> HybridGuard & {
  if (this->mode_ == GuardMode::OPTIMISTIC) { this->ValidateOptimisticLock(); }
  this->lock_  = other.lock_;
  this->mode_  = other.mode_;
  this->state_ = other.state_;
  return *this;
}

/**
 * Destructor -- Depends on the mode_:
 *
 * - GuardMode::OPTIMISTIC: Call ValidateOptimisticLock()
 * - GuardMode::SHARED/EXCLUSIVE: Call Unlock()
 */
HybridGuard::~HybridGuard() noexcept(false) {
  switch (mode_) {
    case GuardMode::OPTIMISTIC: ValidateOptimisticLock(); break;
    default: Unlock(); break;
  }
}

/**
 * Do nothing for Optimistic mode. For Shared and Exclusive, unlock the lock.
 * Make sure to set the mode_ to GuardMode::MOVED to prevent double unlock
 */
void HybridGuard::Unlock() {
  if (mode_ != GuardMode::SHARED && mode_ != GuardMode::EXCLUSIVE) { return; }
  switch (mode_) {
    case GuardMode::SHARED: lock_->UnlockShared(); break;
    case GuardMode::EXCLUSIVE: lock_->UnlockExclusive(); break;
    default: break;
  }
  // Unlock successfully, moved to prevent extra check during guard destruction
  mode_ = GuardMode::MOVED;
}

/* Sleeping until acquire optimistic lock successfully */
void HybridGuard::OptimisticLock() {
  if (mode_ != GuardMode::OPTIMISTIC) { return; }
  state_ = lock_->StateAndVersion().load();
  while (HybridLock::LockState(state_) == HybridLock::EXCLUSIVE) {
    std::this_thread::yield();
    state_ = lock_->StateAndVersion().load();
  }
}

/* Validate the optimistic lock */
void HybridGuard::ValidateOptimisticLock() {
  // This is guard for a shared lock, do nothing
  if (mode_ != GuardMode::OPTIMISTIC) { return; }
  // Done validation, moved to prevent extra check during guard destruction
  mode_             = GuardMode::MOVED;
  auto latest_state = lock_->StateAndVersion().load();
  if (HybridLock::LockState(latest_state) == HybridLock::EXCLUSIVE ||
      HybridLock::Version(latest_state) != HybridLock::Version(state_)) {
    throw RestartException();
  }
}

}  // namespace FinalProject
