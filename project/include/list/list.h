#pragma once

#include "node.h"
#include "sync/epoch.h"
#include "sync/guard.h"

#include <shared_mutex>

namespace FinalProject {
 template <typename T>
class SortedList {
 public:
  SortedList() = default;
  
  auto NewNode(T value, Node<T> *next) -> Node<T> *;
  virtual void Insert(T value)             = 0;
  virtual auto LookUp(T value, T &result) -> bool = 0;
  virtual auto Delete(T value) -> bool                = 0;
};

 template <typename T>
class MutexSortedList : SortedList<T> {
 public:
  MutexSortedList() = default;
  ~MutexSortedList();
  void Insert(T value);
  auto LookUp(T value, T &result) -> bool;
  auto Delete(T value) -> bool;

 private:
  Node<T> *root_{nullptr};
  std::shared_mutex lock_;
};

/**
 * TODO: Your task is to implement the following
 */
 template <typename T>
class OptimisticSortedList : SortedList<T> {
 public:
  OptimisticSortedList(EpochHandler *ep);
  ~OptimisticSortedList();
  void Insert(T value);
  auto LookUp(T value, T &result) -> bool;
  auto Delete(T value) -> bool;

  private:
  Node<T> *root_{nullptr};
  HybridLock lock_;
  EpochHandler *epoch_;
};

}  // namespace FinalProject

