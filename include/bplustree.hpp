#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#ifndef NDEBUG
#include <cassert>
#define BPLUSTREE_ASSERT(x) \
    do {                    \
        assert(x);          \
    } while (0)
#else
#define BPLUSTREE_ASSERT(x) \
    do {                    \
    } while (0)
#endif

#ifdef BPLUSTREE_TESTING_PRIVATE
#define BPLUSTREE_PRIVATE_TESTABLE public
#else
#define BPLUSTREE_PRIVATE_TESTABLE private
#endif

template <typename Key, typename Value>
struct btree_default_traits {
    /**
     * Slots count in each leaf node of the tree. Estimated so that each node has a size of about 256 bytes.
     * Must be correlated with the layout of an leaf node.
     */
    static const int leaf_slots = std::max<int>(8, 256 / (sizeof(Value)));
    /**
     * Slots count in each inner node of the tree. Estimated so that each node has a size of about 256 bytes.
     * Must be correlated with the layout of an inner node.
     */
    static const int inner_slots = std::max<int>(8, (240 - sizeof(void*)) / (sizeof(Key) + sizeof(void*)));
    static const bool debug = false;
};

/**
 * Extract the key from `value` as if the value is the key.
 * @note Useful to build a std::set-like structure on top of `bplustree`.
 */
struct btree_key_extractor_self {
    template<typename T>
    [[nodiscard]] const auto& operator()(const T& value) { return value; }
};

/**
 * Extract the key from `value` as if the value is a pair of key/data.
 * @note Useful to build a std::map-like structure on top of `bplustree`.
 */
struct btree_key_extractor_pair {
    template<typename T>
    [[nodiscard]] const auto& operator()(const T& value) { return value.first; }
};

namespace detail {

/**
 * Generate all comparisons from the comparison function Compare, given than Compare induces a strict weak ordering on
 * its arguments.
 *
 * @see https://en.cppreference.com/w/cpp/named_req/Compare
 *
 * @note The naming of the generated comparison objects assume that `Compare` implements a `less than` relationship,
 * which corresponds to the default comparator of the container class. However, this is not strictly required, and
 * giving for example a comparator implementing a `greater than` relation would cause the semantic to be reversed:
 * less_than() would performs `greater than` and greater_than() would performs `less than`. But the logic would remain
 * intact.
 */

static constexpr struct {
    template <typename Compare, typename T>
    [[nodiscard]] constexpr bool operator()(Compare&& comp, T&& a, T&& b) const {
        return comp(std::forward<T>(a), std::forward<T>(b));
    }
} less_than;

static constexpr struct {
    template <typename Compare, typename T>
    [[nodiscard]] constexpr bool operator()(Compare&& comp, T&& a, T&& b) const {
        return !comp(std::forward<T>(b), std::forward<T>(a));
    }
} less_than_or_equal_to;

static constexpr struct {
    template <typename Compare, typename T>
    [[nodiscard]] constexpr bool operator()(Compare&& comp, T&& a, T&& b) const {
        return comp(std::forward<T>(b), std::forward<T>(a));
    }
} greater_than;

static constexpr struct {
    template <typename Compare, typename T>
    [[nodiscard]] constexpr bool operator()(Compare&& comp, T&& a, T&& b) const {
        return !comp(std::forward<T>(a), std::forward<T>(b));
    }
} greater_than_or_equal_to;

static constexpr struct {
    template <typename Compare, typename T>
    [[nodiscard]] constexpr bool operator()(Compare&& comp, T&& a, T&& b) const {
        return !comp(std::forward<T>(a), std::forward<T>(b)) && !comp(std::forward<T>(b), std::forward<T>(a));
    }
} equal_to;

static constexpr struct {
    template <typename Compare, typename T>
    [[nodiscard]] constexpr bool operator()(Compare&& comp, T&& a, T&& b) const {
        return comp(std::forward<T>(a), std::forward<T>(b)) || comp(std::forward<T>(b), std::forward<T>(a));
    }
} not_equal_to;

}  // namespace detail

template <typename Key,
          typename Value,
          typename KeyExtractor,
          typename Compare = std::less<Key>,
          typename Traits = btree_default_traits<Key, Value>,
          typename Allocator = std::allocator<Value>>
class btree {
BPLUSTREE_PRIVATE_TESTABLE:
    using btree_type = btree<Key, Value, KeyExtractor, Compare, Traits, Allocator>;
    template <typename V>
    class iterator_base;

    struct tree_stats;

public:
    using key_type = Key;
    using value_type = Value;
    using key_extractor_type = KeyExtractor;
    using key_compare_type = Compare;
    using value_compare_type = int;  // TODO
    using traits_type = Traits;
    using allocator_type = Allocator;
    using size_type = size_t;

    using iterator = iterator_base<Value>;
    using const_iterator = iterator_base<const Value>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    explicit btree(const allocator_type& alloc = allocator_type{}) : allocator(alloc) {}

    explicit btree(const key_compare_type& comp, const allocator_type& alloc = allocator_type{})
        : key_compare(comp), allocator(alloc) {}

    template <class InputIterator>
    btree(InputIterator first, InputIterator last, const allocator_type& alloc = allocator_type{}) : allocator(alloc) {
        // insert(first, last);
    }

    template <class InputIterator>
    btree(InputIterator first,
          InputIterator last,
          const key_compare_type& comp,
          const allocator_type& alloc = allocator_type{})
        : key_compare(comp), allocator(alloc) {
        // insert(first, last);
    }

    ~btree() { clear(); }

    [[nodiscard]] iterator begin() noexcept { return iterator(head_leaf, 0); }
    [[nodiscard]] const_iterator begin() const noexcept { return const_iterator(head_leaf, 0); }
    [[nodiscard]] const_iterator cbegin() const noexcept { const_iterator(head_leaf, 0); }
    [[nodiscard]] reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

    [[nodiscard]] iterator end() noexcept { return iterator(tail_leaf, tail_leaf ? tail_leaf->slot_count : 0); }
    [[nodiscard]] const_iterator end() const noexcept {
        return const_iterator(tail_leaf, tail_leaf ? tail_leaf->slot_count : 0);
    }
    [[nodiscard]] const_iterator cend() const noexcept {
        return const_iterator(tail_leaf, tail_leaf ? tail_leaf->slot_count : 0);
    }
    [[nodiscard]] reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    [[nodiscard]] const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    [[nodiscard]] const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

    allocator_type get_allocator() const noexcept { return allocator; }

    void swap(btree& from) {
        std::swap(root, from.root);
        std::swap(head_leaf, from.head_leaf);
        std::swap(tail_leaf, from.tail_leaf);
        std::swap(stats, from.stats);
        std::swap(key_compare, from.key_compare);
        std::swap(allocator, from.allocator);
    }

    void clear() {
        if (root) {
            clear_recursive(root);
            deallocate_node(root);

            root = nullptr;
            head_leaf = tail_leaf = nullptr;

            stats = tree_stats{};
        }
    }

    [[nodiscard]] size_type size() const noexcept { return stats.size; }

    [[nodiscard]] size_type max_size() const noexcept { return std::numeric_limits<size_type>::max(); }

    [[nodiscard]] bool empty() const noexcept { return size() == 0; }

    [[nodiscard]] const tree_stats& get_stats() const noexcept { return stats; }

    template <typename Iterator>
    [[nodiscard]] static Iterator lower_bound_impl(btree_type& self, const key_type& key) {
        node_type* node = self.root;
        if (!node) {
            return self.end();
        }
        while (!node->is_leafnode()) {
            const auto* inner = static_cast<const inner_node_type*>(node);
            slot_type slot = self.find_lower_bound_slot(*inner, key);
            node = inner->childs[slot];
        }
        auto* leaf = static_cast<leaf_node_type*>(node);
        slot_type slot = self.find_lower_bound_slot(*leaf, key);
        return Iterator(leaf, slot);
    }

    [[nodiscard]] iterator lower_bound(const key_type& key) { return lower_bound_impl<iterator>(*this, key); }
    [[nodiscard]] const_iterator lower_bound(const key_type& key) const {
        return lower_bound_impl<const_iterator>(*this, key);
    }

BPLUSTREE_PRIVATE_TESTABLE:
    using level_type = size_type;
    using slot_type = size_type;

    static const slot_type leaf_slots_max = traits_type::leaf_slots;
    static const slot_type inner_slots_max = traits_type::inner_slots;
    static const slot_type leaf_slots_min = leaf_slots_max / 2;
    static const slot_type inner_slots_min = inner_slots_max / 2;

    struct node_type {
        slot_type level{};
        slot_type slot_count{};
        [[nodiscard]] bool is_leafnode() const noexcept { return level == 0; }
        [[nodiscard]] bool is_full() const noexcept { return slot_count == leaf_slots_max; }
        [[nodiscard]] bool is_few() const noexcept { return slot_count <= leaf_slots_min; }
        [[nodiscard]] bool is_underflow() const noexcept { return slot_count < leaf_slots_min; }
    };

    struct inner_node_type : public node_type {
        using alloc_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<inner_node_type>;

        std::array<key_type, inner_slots_max> keys;
        std::array<node_type*, inner_slots_max + 1> childs;

        [[nodiscard]] const key_type& key(slot_type slot) const noexcept { return keys[slot]; }
    };

    struct leaf_node_type : public node_type {
        using alloc_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<leaf_node_type>;

        leaf_node_type* previous_leaf{};
        leaf_node_type* next_leaf{};

        std::array<value_type, leaf_slots_max> data;

        [[nodiscard]] const key_type& key(slot_type slot) const { return key_extractor_type{}(data[slot]); }
    };

    struct tree_stats {
        size_type size{};
        size_type leaves{};
        size_type inner_nodes{};

        [[nodiscard]] size_type nodes() const noexcept { return inner_nodes + leaves; }

        template <typename T = float>
        [[nodiscard]] T average_fill_leaves() const noexcept {
            return static_cast<T>(size) / (leaves * leaf_slots_max);
        }
    };

    template <typename Node, typename... Args>
    static Node* allocate_from_allocator(const allocator_type& allocator, Args&&... args) {
        auto alloc = typename Node::alloc_type(allocator);
        auto* n = std::allocator_traits<decltype(alloc)>::allocate(alloc, 1);
        std::allocator_traits<decltype(alloc)>::construct(alloc, n, std::forward<Args>(args)...);
        return n;
    }

    template <typename Node>
    static void deallocate_from_allocator(const allocator_type& allocator, Node* n) {
        auto alloc = typename Node::alloc_type(allocator);
        std::allocator_traits<decltype(alloc)>::destroy(alloc, n);
        std::allocator_traits<decltype(alloc)>::deallocate(alloc, n, 1);
    }

    [[nodiscard]] leaf_node_type* allocate_leaf() {
        auto* leaf = allocate_from_allocator<leaf_node_type>(allocator);
        ++stats.leaves;
        return leaf;
    }

    [[nodiscard]] inner_node_type* allocate_inner(slot_type level) {
        auto* inner = allocate_from_allocator<inner_node_type>(allocator, level);
        ++stats.inner_nodes;
        return inner;
    }

    void deallocate_node(node_type* n) {
        if (n->is_leafnode()) {
            deallocate_from_allocator(allocator, static_cast<leaf_node_type*>(n));
            --stats.leaves;
        } else {
            deallocate_from_allocator(allocator, static_cast<inner_node_type*>(n));
            --stats.inner_nodes;
        }
    }

    void clear_recursive(node_type* n) {
        BPLUSTREE_ASSERT(n != nullptr);
        if (n->is_leafnode()) {
            // data objects are deleted by leaf_node's destructor
        } else {
            auto* inner = static_cast<inner_node_type*>(n);
            for (slot_type slot = 0; slot < inner->slot_count + 1; ++slot) {
                clear_recursive(inner->childs[slot]);
                deallocate_node(inner->childs[slot]);
            }
        }
    }

    /**
     * Find the slot corresponding to `key` for the given `node`. All comparisons on keys are performed using the
     * comparison object `comp`.
     *
     * @todo Perform a linear search for small number of key slots. Threshold between linear and binary search should be
     * configurable by the user.
     */
    template <typename Node, typename Comparator>
    [[nodiscard]] slot_type find_slot(const Node& node, const Comparator& comp, const key_type& key) const noexcept {
        slot_type lower = 0, upper = node.slot_count;
        while (lower < upper) {
            const slot_type middle = (lower + upper) / 2;
            if (comp(key_compare, node.key(middle), key)) {
                upper = middle;
            } else {
                lower = middle + 1;
            }
        }
        return lower;
    }

    template <typename Node>
    [[nodiscard]] slot_type find_lower_bound_slot(const Node& node, const key_type& key) const noexcept {
        return find_slot(node, detail::greater_than_or_equal_to, key);
    }

    template <typename Node>
    [[nodiscard]] slot_type find_upper_bound_slot(const Node& node, const key_type& key) const noexcept {
        return find_slot(node, detail::greater_than, key);
    }

    node_type* root{};
    leaf_node_type* head_leaf{};
    leaf_node_type* tail_leaf{};
    tree_stats stats;
    // key_extractor_type key_extractor; // Cannot be stored for now since it is used inside node types
    key_compare_type key_compare;
    allocator_type allocator;

    template <typename V>
    class iterator_base {
    public:
        using key_type = Key;
        using value_type = std::decay_t<V>;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;

        using leaf_node_type_ = std::conditional_t<std::is_const_v<value_type>, const leaf_node_type, leaf_node_type>;

        iterator_base() noexcept = default;
        iterator_base(leaf_node_type_* l, slot_type s) noexcept : current_leaf{l}, current_slot{s} {}

        [[nodiscard]] reference operator*() const noexcept { return current_leaf->data[current_slot]; }
        [[nodiscard]] pointer operator->() const noexcept { return &current_leaf->data[current_slot]; }

        [[nodiscard]] const key_type& key() const noexcept { return current_leaf->key(current_slot); }

        iterator_base& operator++() noexcept {
            next();
            return *this;
        }

        [[nodiscard]] iterator_base operator++(int) noexcept {
            iterator_base tmp = *this;
            next();
            return tmp;
        }

        iterator_base& operator--() noexcept {
            previous();
            return *this;
        }

        [[nodiscard]] iterator_base operator--(int) noexcept {
            iterator_base tmp = *this;
            previous();
            return tmp;
        }

        [[nodiscard]] bool operator==(const iterator_base& x) const noexcept {
            return x.current_leaf == current_leaf && x.current_slot == current_slot;
        }
        [[nodiscard]] bool operator!=(const iterator_base& x) const noexcept {
            return x.current_leaf != current_leaf || x.current_slot != current_slot;
        }

    BPLUSTREE_PRIVATE_TESTABLE:
        void next() noexcept {
            BPLUSTREE_ASSERT(current_leaf != nullptr);
            if (current_slot + 1u < current_leaf->slot_count) {
                // There is still data in current node, switching to next slot
                ++current_slot;
            } else if (current_leaf->next_leaf != nullptr) {
                // No data on current node, switching to the next one
                current_leaf = current_leaf->next_leaf;
                current_slot = 0;
            } else {
                // No data and no node left, setting current slot to end()
                current_slot = current_leaf->slot_count;
            }
        }

        void previous() noexcept {
            BPLUSTREE_ASSERT(current_leaf != nullptr);
            if (current_slot > 0) {
                // There is still data in current node, switching to previous slot
                --current_slot;
            } else if (current_leaf->previous_leaf != nullptr) {
                // No data on current node, switching to the previous one
                current_leaf = current_leaf->previous_leaf;
                current_slot = current_leaf->slot_count - 1;
            } else {
                // No node left, setting current slot to begin()
                current_slot = 0;
            }
        }

        leaf_node_type_* current_leaf{};
        slot_type current_slot{};
    };
};
