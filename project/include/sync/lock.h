#pragma once

#include <atomic>

namespace FinalProject {

class HybridLock {
 public:
  static constexpr uint64_t VERSION_MASK = (static_cast<uint64_t>(1) << 56) - 1;
  static constexpr uint64_t UNLOCKED     = 0;
  static constexpr uint64_t MAX_SHARED   = 254;  // # share-lock holders (1 -> MAX_SHARED)
  static constexpr uint64_t EXCLUSIVE    = 255;

  HybridLock();
  ~HybridLock()                      = default;
  auto operator=(const HybridLock &) = delete;  // No COPY constructor
  auto operator=(HybridLock &&)      = delete;  // No MOVE constructor

  // State utilities
  static auto LockState(uint64_t v) -> uint64_t;
  static auto Version(uint64_t v) -> uint64_t;
  auto LockState() -> uint64_t;
  auto Version() -> uint64_t;
  auto StateAndVersion() -> std::atomic<uint64_t> &;

  // Lock utilities
  auto TryLockExclusive(uint64_t old_state_w_version) -> bool;
  void UnlockExclusive();
  auto TryLockShared(uint64_t old_state_w_version) -> bool;
  void UnlockShared();
  auto UpgradeLock(uint64_t old_state_w_version) -> bool;
  void DowngradeLock();

 protected:
  friend class HybridGuard;

  // Assertion utilities
  static auto IsExclusivelyLocked(uint64_t v) -> bool { return LockState(v) == EXCLUSIVE; }

  static auto IsSharedLocked(uint64_t v) -> bool {
    auto curr = LockState(v);
    return (UNLOCKED < curr) && (curr < EXCLUSIVE);
  }

  // State & Version utilities
  static auto SameVersionNewState(uint64_t old_state_and_version, uint64_t new_state) -> uint64_t {
    return static_cast<uint64_t>(((old_state_and_version << 8) >> 8) | new_state << 56);
  }

  static auto NextVersionNewState(uint64_t old_state_and_version, uint64_t new_state) -> uint64_t {
    return static_cast<uint64_t>((((old_state_and_version << 8) >> 8) + 1) | new_state << 56);
  }

  std::atomic<uint64_t> state_and_version_;
};

}  // namespace FinalProject