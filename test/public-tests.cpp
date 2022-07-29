#include <bplustree.hpp>

#include <gtest/gtest.h>

template <typename T>
using set = btree<T, T, btree_key_extractor_self>;

/**
 * Lower bound
 */
TEST(lower_bound, empty) {
    set<int> tree;
    auto it = tree.lower_bound(3);
    ASSERT_EQ(it, tree.end());
}

TEST(lower_bound_const, empty) {
    const set<int> tree;
    auto it = tree.lower_bound(3);
    ASSERT_EQ(it, tree.end());
}

/**
 * Upper bound
 */

TEST(upper_bound, empty) {
    set<int> tree;
    auto it = tree.upper_bound(3);
    ASSERT_EQ(it, tree.end());
}

TEST(upper_bound_const, empty) {
    const set<int> tree;
    auto it = tree.upper_bound(3);
    ASSERT_EQ(it, tree.end());
}

/**
 * Iterators
 */

// Check iterator/const_iterator conversions
static_assert(std::is_convertible_v<set<int>::iterator, set<int>::const_iterator>);
static_assert(!std::is_convertible_v<set<int>::const_iterator, set<int>::iterator>);
static_assert(std::is_convertible_v<set<int>::reverse_iterator, set<int>::const_reverse_iterator>);
static_assert(!std::is_convertible_v<set<int>::const_reverse_iterator, set<int>::reverse_iterator>);

TEST(iterator, non_const_to_const_conversion_begin) {
    set<int> tree;
    set<int>::iterator begin = tree.begin();
    set<int>::const_iterator cbegin = begin;
    ASSERT_EQ(cbegin, tree.cbegin());
}

TEST(reverse_iterator, non_const_to_const_conversion_begin) {
    set<int> tree;
    set<int>::reverse_iterator rbegin = tree.rbegin();
    set<int>::const_reverse_iterator crbegin = rbegin;
    ASSERT_EQ(crbegin, tree.crbegin());
}
