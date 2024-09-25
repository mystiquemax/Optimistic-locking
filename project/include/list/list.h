#pragma once

#include "node.h"
#include "sync/epoch.h"
#include "sync/guard.h"

#include <shared_mutex>

namespace FinalProject {

class SortedList {
 public:
  SortedList() = default;
  auto NewNode(int key, int value, Node *next) -> Node *;
  virtual void Insert(int key, int value)              = 0;
  virtual auto LookUp(int key, int &out_value) -> bool = 0;
  virtual auto Delete(int key) -> bool                 = 0;
};

class MutexSortedList : SortedList {
 public:
  MutexSortedList() = default;
  ~MutexSortedList();

  void Insert(int key, int value);
  auto LookUp(int key, int &out_value) -> bool;
  auto Delete(int key) -> bool;

 private:
  Node *root_{nullptr};
  std::shared_mutex lock_;
};

/**
 * TODO: Your task is to implement the following
 */
class OptimisticSortedList : SortedList {
 public:
  OptimisticSortedList(EpochHandler *ep);
  ~OptimisticSortedList();

  void Insert(int key, int value);
  auto LookUp(int key, int &out_value) -> bool;
  auto Delete(int key) -> bool;

 private:
  Node *root_{nullptr};
  HybridLock lock_;
  EpochHandler *epoch_;
};

}  // namespace FinalProject
