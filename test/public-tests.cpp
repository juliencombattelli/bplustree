#include <bplustree.hpp>

#include <gtest/gtest.h>

template <typename T>
using set = btree<T, T, btree_key_extractor_self>;

TEST(lower_bound, empty) {
    set<int> tree;
    auto it = tree.lower_bound(3);
    ASSERT_EQ(it, tree.end());
}

TEST(upper_bound, empty) {
    set<int> tree;
    auto it = tree.upper_bound(3);
    ASSERT_EQ(it, tree.end());
}