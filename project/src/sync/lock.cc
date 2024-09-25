#include "sync/lock.h"

#include <cassert>
#include <stdexcept>

namespace FinalProject {

HybridLock::HybridLock() { state_and_version_.store(SameVersionNewState(0, UNLOCKED)); }

auto HybridLock::LockState(uint64_t v) -> uint64_t { return v >> 56; }

auto HybridLock::Version(uint64_t v) -> uint64_t { return static_cast<uint64_t>(v & VERSION_MASK); }

/* LockState() of the current lock */
auto HybridLock::LockState() -> uint64_t { return LockState(state_and_version_.load()); }

/* Version counter of the current lock */
auto HybridLock::Version() -> uint64_t { return Version(state_and_version_.load()); }

/* Return reference to the atomic cnt of this lock */
auto HybridLock::StateAndVersion() -> std::atomic<uint64_t> & { return state_and_version_; }

/* Try to acquire exclusive lock. Return false if unsuccesful, true otherwise */
auto HybridLock::TryLockExclusive(uint64_t old_state_w_version) -> bool {
  if (LockState(old_state_w_version) > UNLOCKED && LockState(old_state_w_version) <= EXCLUSIVE) { return false; }
  return state_and_version_.compare_exchange_strong(old_state_w_version,
                                                    SameVersionNewState(old_state_w_version, EXCLUSIVE));
}

/* Unlock exclusive -- must check that the lock is in exclusive mode before unlocking it */
void HybridLock::UnlockExclusive() {
  assert(IsExclusivelyLocked(StateAndVersion()));
  state_and_version_.store(NextVersionNewState(state_and_version_.load(), UNLOCKED), std::memory_order_release);
}

/* Downgrade from exclusive -> shared. */
void HybridLock::DowngradeLock() {
  assert(IsExclusivelyLocked(StateAndVersion()));
  // 1 here means 1 reader
  state_and_version_.store(NextVersionNewState(state_and_version_.load(), 1), std::memory_order_release);
}

/* Lock in shared mode. Current lock must not in exclusive mode */
auto HybridLock::TryLockShared(uint64_t old_state_w_version) -> bool {
  auto state = LockState(old_state_w_version);

  // Reach maximum number of readers
  if (state < MAX_SHARED) {
    return state_and_version_.compare_exchange_strong(old_state_w_version,
                                                      SameVersionNewState(old_state_w_version, state + 1));
  }

  return false;
}

/* Upgrade from shared -> exclusive.
   Make sure that there is only one SHARED lock holder -- LockState(old_state_w_version) == 1 */
auto HybridLock::UpgradeLock(uint64_t old_state_w_version) -> bool {
  if (LockState(old_state_w_version) != 1) {
    // Only upgrade from shared -> exclusive if there is one reader
    return false;
  }
  return state_and_version_.compare_exchange_weak(old_state_w_version,
                                                  SameVersionNewState(old_state_w_version, EXCLUSIVE));
}

/* Unlock shared mode. Decrease the LockState() by 1. Must not be in exclusive lock */
void HybridLock::UnlockShared() {
  while (true) {
    auto old_state_w_version = state_and_version_.load();
    auto old_state           = LockState(old_state_w_version);
    assert(IsSharedLocked(old_state_w_version));
    if (state_and_version_.compare_exchange_weak(old_state_w_version,
                                                 SameVersionNewState(old_state_w_version, old_state - 1))) {
      break;
    }
  }
}

}  // namespace FinalProject