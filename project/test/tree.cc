#include "splaytree/tree.h"

#include "common/utest.h"

#include <random>
#include <thread>

#define NO_ENTRIES 10000
static constexpr int NO_THREADS = 10;

using SplayTree = FinalProject::SplayTree<int>;
using TreeNode  = SplayTree::TreeNode;

constexpr auto Comparison{[](TreeNode *lhs, TreeNode *rhs) -> int { return lhs->key - rhs->key; }};

UTEST(SplayTree, InsertAndSearch) {
  auto tree = SplayTree(Comparison, 1);
  TreeNode data[NO_ENTRIES];
  for (int i = 0; i < NO_ENTRIES; i++) {
    data[i].key = i * 2 + 1;
    ASSERT_TRUE(tree.Insert(&data[i]));
  }

  TreeNode query;
  TreeNode *result;
  TreeNode *cur;

  for (int i = 0; i < 2 * NO_ENTRIES; i++) {
    query.key = i;
    cur       = tree.Search(&query);
    if (i % 2) {
      ASSERT_EQ(cur->key, i);
      ASSERT_EQ(cur, tree.RootPtr());
    } else {
      ASSERT_EQ(cur, nullptr);
    }
  }
}

UTEST(SplayTree, RemoveAndLowerBound) {
  auto tree = SplayTree(Comparison, 1);
  TreeNode data[NO_ENTRIES];
  for (int i = 0; i < NO_ENTRIES; i++) {
    data[i].key = i;
    ASSERT_TRUE(tree.Insert(&data[i]));
  }

  bool already_deleted[NO_ENTRIES + 1] = {false};

  TreeNode query;
  TreeNode *result;
  TreeNode *cur;
  for (int i = 0; i < NO_ENTRIES * 1.5; i++) {
    query.key = rand() % (NO_ENTRIES);
    cur       = tree.Search(&query);

    if (already_deleted[query.key]) {
      ASSERT_EQ(cur, nullptr);
    } else {
      already_deleted[query.key] = true;
      ASSERT_NE(cur, nullptr);
      ASSERT_TRUE(tree.Delete(&query));
      cur = tree.Search(&query);
      ASSERT_EQ(cur, nullptr);
    }

    cur = tree.LowerBound(&query);
    if (cur) { ASSERT_GT(cur->key, query.key); }
  }
}

UTEST(SplayTree, MultiThreadInsert) {
  UTEST_SKIP("Unskip if Concurrent SplayTree is implemented.");

  FinalProject::RegisterSegfaultHandler();

  auto tree = SplayTree(Comparison, NO_THREADS);
  std::vector<int> data{NO_ENTRIES};
  for (int i = 0; i < NO_ENTRIES; i++) { data.push_back(i); }

  std::thread threads[NO_THREADS];
  for (int idx = 0; idx < NO_THREADS; idx++) {
    threads[idx] = std::thread([&]() {
      FinalProject::InitializeThread();
      ASSERT_GT(FinalProject::thread_id, 0);
      std::vector<int> to_insert_data(data);
      shuffle(to_insert_data.begin(), to_insert_data.end(), std::mt19937(FinalProject::thread_id));

      for (auto &new_data : to_insert_data) {
        auto new_node = new TreeNode();
        new_node->key = new_data;
        if (!tree.Insert(new_node)) { delete (new_node); }
      }
    });
  }

  for (auto &thread : threads) { thread.join(); }

  TreeNode query;
  for (int i = 0; i < 2 * NO_ENTRIES; i++) {
    query.key = i;
    auto cur  = tree.Search(&query);
    if (i <= NO_ENTRIES) {
      ASSERT_EQ(cur->key, i);
      ASSERT_EQ(cur, tree.RootPtr());
    } else {
      ASSERT_EQ(cur, nullptr);
    }
  }
}

UTEST_MAIN();
