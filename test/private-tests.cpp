#include <bplustree.hpp>

#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

template <typename T>
using set = btree<T, T, btree_key_extractor_self>;

// TEST(inner_node, lower_bound) {
//     set<int>::inner_node_type node;
//     set<int> tree;

//     node.keys = {0, 1, 2, 3, 3, 3, 4, 5, 6};
//     node.slot_count = 9;
//     auto slot = set<int>::find_slot_in_node(tree, node, detail::greater_than_or_equal_to, 3);
//     ASSERT_EQ(slot, 3);
// }

// TEST(inner_node, upper_bound) {
//     set<int>::inner_node_type node;
//     set<int> tree;

//     node.keys = {0, 1, 2, 3, 3, 3, 4, 5, 6};
//     node.slot_count = 9;
//     auto slot = set<int>::find_slot_in_node(tree, node, detail::greater_than, 3);
//     ASSERT_EQ(slot, 6);
// }

// TEST(leaf_node, lower_bound) {
//     set<int>::leaf_node_type node;
//     set<int> tree;

//     node.data = {10, 11, 12, 13, 13, 13, 14, 15, 16};
//     node.slot_count = 9;
//     auto slot = set<int>::find_slot_in_node(tree, node, detail::greater_than_or_equal_to, 3);
//     std::cout << "slot found: " << slot << "\n";
//     ASSERT_EQ(slot, 3);
// }

// TEST(leaf_node, upper_bound) {
//     set<int>::leaf_node_type node;
//     set<int> tree;

//     node.data = {0, 1, 2, 3, 3, 3, 4, 5, 6};
//     node.slot_count = 9;
//     auto slot = set<int>::find_slot_in_node(tree, node, detail::greater_than, 3);
//     ASSERT_EQ(slot, 6);
// }
