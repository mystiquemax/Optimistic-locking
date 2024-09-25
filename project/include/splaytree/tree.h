#pragma once

#include <cstdint>
#include <cstdio>
#include <functional>

#include "common/utils.h"
#include "sync/epoch.h"
#include "sync/guard.h"
#include "sync/lock.h"

namespace FinalProject {

#define GET_ENTRY(ELEM, STRUCT, MEMBER) \
  (reinterpret_cast<STRUCT *>(reinterpret_cast<uint8_t *>(ELEM) - offsetof(STRUCT, MEMBER)))

template <typename T>
class SplayTree {
 public:
  struct TreeNode {
    T key{};
    TreeNode *left{nullptr};
    TreeNode *right{nullptr};
  };

  using ComparisonFn = std::function<int(TreeNode *lhs, TreeNode *rhs)>;

  SplayTree(const ComparisonFn &cmp_fn, int number_of_threads);
  ~SplayTree() = default;

  auto RootPtr() -> void *;

  /* Main APIs */

  /* Assignment 1 */
  auto Insert(TreeNode *node) -> bool;
  auto Search(TreeNode *node) -> TreeNode *;
  auto LowerBound(TreeNode *node) -> TreeNode *;
  auto Delete(TreeNode *node) -> bool;

  /* Assignment 2 */
  auto LockFreeSearch(TreeNode *node) -> TreeNode *;

 private:
  /* For balancing */
  auto Splay(TreeNode *query, ComparisonFn &func, int &out_cmp_ret) -> TreeNode *;
  auto RightRotate(TreeNode *x) -> TreeNode *;
  auto LeftRotate(TreeNode *x) -> TreeNode *;

  /* Main properties */
  TreeNode *root_{nullptr};
  ComparisonFn cmp_func_;

  /* Assignment 2 */
  HybridLock lock_;
  EpochHandler epoch_;
};

template <typename T>
SplayTree<T>::SplayTree(const ComparisonFn &cmp_fn, int number_of_threads)
    : cmp_func_(cmp_fn), epoch_(number_of_threads) {}

template <typename T>
auto SplayTree<T>::RootPtr() -> void * {
  return reinterpret_cast<void *>(root_);
}

/**
 * @brief Splay operation -- move a tree node which func(query, to-splay-node) == 0
 * If there is no such to-splay-node that func(query, to-splay-node) == 0,
 *  then to-splay-node should be the node at the end of the search
 *
 * Example:
 *       8
 *      / \
 *     3   10
 *    / \
 *   1   5
 *
 * Assume `TreeNode *query` contains value 4.
 * During tree traversal, we would end up with the TreeNode containing 5 ("to-splay-node")
 * As the result, we're going to "splay" that node to the root of the tree and then return it
 *
 * @param query The tree node we're want to "splay"
 * @param func Comparison function
 * @param cmp_ret The comparison output of the "to-splay-node" with query
 *
 * @return TreeNode* to-splay-node
 */
template <typename T>
auto SplayTree<T>::Splay(TreeNode *query, ComparisonFn &func, int &cmp_ret) -> TreeNode * {
  if (!root_) { return root_; }
  TreeNode N;
  TreeNode *left_t;
  TreeNode *right_t;
  left_t = right_t = &N;

  for (;;) {
    cmp_ret = func(root_, query);
    if (cmp_ret == 0) { break; }

    if (cmp_ret > 0) {
      if (!root_->left) { break; }

      if (func(root_->left, query) > 0) {
        root_ = RightRotate(root_);
        if (!root_->left) { break; }
      }
      right_t->left = root_;
      right_t       = root_;
      root_         = root_->left;
    } else {
      if (!root_->right) { break; }

      if (func(root_->right, query) < 0) {
        root_ = LeftRotate(root_);
        if (!root_->right) { break; }
      }

      left_t->right = root_;
      left_t        = root_;
      root_         = root_->right;
    }
  }

  left_t->right = root_->left;
  right_t->left = root_->right;
  root_->left   = N.right;
  root_->right  = N.left;
  return root_;
}

template <typename T>
auto SplayTree<T>::RightRotate(TreeNode *x) -> TreeNode * {
  auto y   = x->left;
  x->left  = y->right;
  y->right = x;
  return y;
}

template <typename T>
auto SplayTree<T>::LeftRotate(TreeNode *x) -> TreeNode * {
  auto y   = x->right;
  x->right = y->left;
  y->left  = x;
  return y;
}

template <typename T>
auto SplayTree<T>::Insert(TreeNode *node) -> bool {
  if (root_ == nullptr) {
    root_ = node;
    return true;
  }
  auto cmp = 0;
  root_    = Splay(node, cmp_func_, cmp);
  if (cmp == 0) { return false; }
  if (cmp > 0) {
    node->right = root_;
    node->left  = root_->left;
    root_->left = NULL;
  } else {
    node->left   = root_;
    node->right  = root_->right;
    root_->right = NULL;
  }
  root_ = node;
  return true;
}

template <typename T>
auto SplayTree<T>::Search(TreeNode *node) -> TreeNode * {
  auto cmp = 0;
  root_    = Splay(node, cmp_func_, cmp);
  if (cmp == 0) { return root_; }

  return NULL;
}

template <typename T>
auto SplayTree<T>::LowerBound(TreeNode *node) -> TreeNode * {
  if (!root_) { return nullptr; }

  int cmp = 0;
  root_   = Splay(node, cmp_func_, cmp);
  if (cmp >= 0) { return root_; }

  if (node->right) { goto move_next; }
  int not_used;
  root_ = Splay(node, cmp_func_, not_used);

move_next:
  TreeNode *p;
  for (p = node->right; p && p->left; p = p->left) {}
  return p;
}

template <typename T>
auto SplayTree<T>::Delete(TreeNode *node) -> bool {
  if (!root_) { return false; };

  int cmp = 0;
  root_   = Splay(node, cmp_func_, cmp);
  if (cmp != 0) { return false; }

  if (!root_->left) {
    root_ = root_->right;
  } else {
    TreeNode **root = &root_;
    // splay the biggest node of the left subtree to the top, and then attach
    // current right-subtree to that node
    TreeNode *pp = nullptr;
    TreeNode *p;
    for (p = (*root)->left; p->right; p = p->right) { pp = p; }
    if (pp) {
      pp->right     = p->left;
      p->left       = (*root)->left;
      (*root)->left = p;
    }

    p->right = (*root)->right;
    *root    = p;
  }

  return true;
}

}  // namespace FinalProject
