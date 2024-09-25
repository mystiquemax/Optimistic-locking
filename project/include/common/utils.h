#pragma once

#include <signal.h>
#include <atomic>

namespace FinalProject {

extern thread_local int thread_id;

class RestartException {
 public:
  RestartException()  = default;
  ~RestartException() = default;
};

void InitializeThread();
void HandleSegfault(int signo, siginfo_t *info, void *extra);
void RegisterSegfaultHandler();

}  // namespace FinalProject