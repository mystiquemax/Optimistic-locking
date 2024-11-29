#include "list/list.h"
#include <cstdint>
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <mutex>
#include "sync/epoch.h"
#include "sync/lock.h"
#include "common/utils.h"

namespace FinalProject {

template <typename T>
auto SortedList<T>::NewNode(T value, Node<T> *next) -> Node<T>* {
  auto memory = malloc(sizeof(Node<T>));
  return new (memory) Node<T>(value, next);
}

template <typename T>
MutexSortedList<T>::~MutexSortedList() {
  Node<T> *tmp;
  for (; root_ != nullptr; root_ = tmp) {
    tmp = root_->next;
    free(root_);
  }
}
template <typename T>
void MutexSortedList<T>::Insert(T value) {
  std::unique_lock guard(lock_);
  if (root_ == nullptr || root_->value <=> value > 0) {
    root_ = this->NewNode(value, root_);
    return;
  }
  bool found = false;
  Node<T> *prev = nullptr;
  for (auto current = root_; current != nullptr; current = current->next) {
    if (current->value <=> value > 0) { break; }
    if (current->value <=> value == 0) {
      found = true;
      break;
    }
    prev = current;
  }
  if (found) {
    assert(prev->next != nullptr);
    prev->next->value = value;
  } else {
    prev->next = this->NewNode(value, prev->next);
  }
}
template <typename T>
auto MutexSortedList<T>::LookUp(T value, T &result) -> bool {
  std::shared_lock guard(lock_);
  bool found = false;
  for (auto current = root_; current != nullptr; current = current->next) {
    if (current->value <=> value > 0) { break; }
    if (current->value  <=> value == 0) {
      found     = true;
      result = current->value;
      break;
    }
  }

  return found;
}
template <typename T>
auto MutexSortedList<T>::Delete(T value) -> bool {
  std::unique_lock guard(lock_);

  bool found = false;
  Node<T> *prev = nullptr;
  Node<T> *current;
  for (current = root_; current != nullptr; current = current->next) {
    if (current->value <=> value > 0) { break; }
    if (current->value <=> value == 0) {
      found = true;
      if (prev == nullptr) {
        root_ = current->next;
      } else {
        prev->next = current->next;
      }
      break;
    }
    prev = current;
  }

  if (found) {
    assert(current != nullptr);
    free(current);
  }
  return found;
}

template <typename T>
OptimisticSortedList<T>::OptimisticSortedList(EpochHandler *ep) : epoch_(ep) {}

template <typename T>
OptimisticSortedList<T>::~OptimisticSortedList() {
  Node<T> *tmp;
  for (; root_ != nullptr; root_ = tmp) {
    tmp = root_->next;
    free(root_);
  }
}

template <typename T>
void OptimisticSortedList<T>::Insert(T value) { 
  HybridGuard guard(&lock_, GuardMode::EXCLUSIVE);
  if (root_ == nullptr || root_->value <=> value > 0) {
    root_ = this->NewNode( value, root_);
    return;
  }
  bool found = false;
  Node<T> *prev = nullptr;
  for (auto current = root_; current != nullptr; current = current->next) {
    if (current->value <=> value > 0) { break; }
    if (current->value <=> value == 0) {
      found = true;
      break;
    }
    prev = current;
  }
  if (found) {
    assert(prev->next != nullptr);
    prev->next->value = value;
  } else {
    prev->next = this->NewNode(value, prev->next);
  }
}

template <typename T>
auto OptimisticSortedList<T>::LookUp(T value, T &result) -> bool { 
   
  while(true){ 
    try{
     EpochGuard epoch_guard(&(epoch_->local_epoch[thread_id]), epoch_->global_epoch); 
     HybridGuard hybrid_guard(&lock_, GuardMode::OPTIMISTIC);
     bool found = false;
      for (auto current = root_; current != nullptr; current = current->next) {
        if (current->value <=> value > 0) { break; }
        if (current->value <=> value == 0) {
           found     = true;
           result = current->value;
           break;
        }
      }
       return found;
    } catch (const RestartException &) {
      //std::cout << "LookUp failed" << std::endl;
    }
  }
}

template <typename T>
auto OptimisticSortedList<T>::Delete(T value) -> bool {   
  HybridGuard guard(&lock_, GuardMode::EXCLUSIVE);
  bool found = false;
  Node<T> *prev = nullptr;
  Node<T> *current;
  for (current = root_; current != nullptr; current = current->next) {
    if (current->value <=> value > 0) { break; }
    if (current->value <=> value == 0) {
      found = true;
      if (prev == nullptr) {
        root_ = current->next;
      } else {
        prev->next = current->next;
      }
      break;
    }
    prev = current;
  }

  if(found){
    assert(current != nullptr);
    epoch_->DeferFreePointer(thread_id, current);
  }

  return found;
}
}  // namespace FinalProject
