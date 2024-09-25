#include "common/utils.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <exception>
#include <stdexcept>

namespace FinalProject {

std::atomic<int> worker_atomic_int = 0;

thread_local int thread_id;

void InitializeThread() { thread_id = ++worker_atomic_int; }

void HandleSegfault(int signo, siginfo_t *info, void *extra) { throw RestartException(); }

void RegisterSegfaultHandler() {
  struct sigaction action;
  action.sa_flags     = SA_SIGINFO;
  action.sa_sigaction = HandleSegfault;
  if (sigaction(SIGSEGV, &action, nullptr) == -1) { throw std::runtime_error("Can't register segfault handler"); }
}

}  // namespace FinalProject