#include "list/list.h"

#include <cassert>
#include <cstdlib>
#include <mutex>

namespace FinalProject {

auto SortedList::NewNode(int key, int value, Node *next) -> Node * {
  auto memory = malloc(sizeof(Node));
  return new (memory) Node(key, value, next);
}

MutexSortedList::~MutexSortedList() {
  Node *tmp;
  for (; root_ != nullptr; root_ = tmp) {
    tmp = root_->next;
    free(root_);
  }
}

void MutexSortedList::Insert(int key, int value) {
  std::unique_lock guard(lock_);
  if (root_ == nullptr || root_->key > key) {
    root_ = NewNode(key, value, root_);
    return;
  }
  bool found = false;
  Node *prev = nullptr;
  for (auto current = root_; current != nullptr; current = current->next) {
    if (current->key > key) { break; }
    if (current->key == key) {
      found = true;
      break;
    }
    prev = current;
  }
  if (found) {
    assert(prev->next != nullptr);
    prev->next->value = value;
  } else {
    prev->next = NewNode(key, value, prev->next);
  }
}

auto MutexSortedList::LookUp(int key, int &out_value) -> bool {
  std::shared_lock guard(lock_);
  bool found = false;
  for (auto current = root_; current != nullptr; current = current->next) {
    if (current->key > key) { break; }
    if (current->key == key) {
      found     = true;
      out_value = current->value;
      break;
    }
  }

  return found;
}

auto MutexSortedList::Delete(int key) -> bool {
  std::unique_lock guard(lock_);

  bool found = false;
  Node *prev = nullptr;
  Node *current;
  for (current = root_; current != nullptr; current = current->next) {
    if (current->key > key) { break; }
    if (current->key == key) {
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

OptimisticSortedList::OptimisticSortedList(EpochHandler *ep) : epoch_(ep) {}

OptimisticSortedList::~OptimisticSortedList() {
  Node *tmp;
  for (; root_ != nullptr; root_ = tmp) {
    tmp = root_->next;
    free(root_);
  }
}

/**
 * TODO: Similar to MutexSortedList::Insert()
 */
void OptimisticSortedList::Insert(int key, int value) { throw std::logic_error("Not yet implemented"); }

/**
 * TODO: LookUp: Similar to MutexSortedList::LookUp(), but with an optimistic lock instead
 *
 * Pseudo-code:
 * Loop infinitely, i.e. while(true) {
 *  try {
 *  - Declare both epoch guard and an optimistic guard - see test/epoch.cc how to do it
 *  - Execute the look up operation similar to MutexSortedList::LookUp()
 *  - Return the result
 *  } catch (const sync::RestartException &) {}
 * }
 *
 * You should try to explain why it works
 */
auto OptimisticSortedList::LookUp(int key, int &out_value) -> bool { throw std::logic_error("Not yet implemented"); }

/**
 * TODO: Similar to MutexSortedList::Delete()
 *
 * Only difference is that, instead of physically delete the node using free(),
 *  you used EpochHandler::DeferFreePointer()
 */
auto OptimisticSortedList::Delete(int key) -> bool { throw std::logic_error("Not yet implemented"); }

}  // namespace FinalProject
