#pragma once

#include "sync/lock.h"

namespace FinalProject {

enum class GuardMode { OPTIMISTIC, SHARED, EXCLUSIVE, MOVED };

class HybridGuard {
 public:
  HybridGuard(HybridLock *lock, GuardMode mode);
  auto operator=(HybridGuard &&other) noexcept(false) -> HybridGuard &;
  ~HybridGuard() noexcept(false);

  void Unlock();

  void OptimisticLock();
  void ValidateOptimisticLock();

 private:
  HybridLock *lock_;
  GuardMode mode_;
  uint64_t state_{0};
};

}  // namespace FinalProject