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

template <typename Key, typename Value>
struct btree_default_traits {
    static const int leaf_slots = std::max<int>(8, 256 / (sizeof(Value)));
    static const int inner_slots = std::max<int>(8, 256 / (sizeof(Key) + sizeof(void*)));
    static const bool debug = false;
};

template <typename Key, typename Value>
struct btree_key_extractor_single {  // TODO could be lambda
    [[nodiscard]] const Key& operator()(const Value& v) { return v; }
};

template <typename Key, typename Value>
struct btree_key_extractor_pair {
    [[nodiscard]] const Key& operator()(const Value& v) { return v.first; }
};

namespace details {

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

}  // namespace details

template <typename Key,
          typename Value,
          typename KeyExtractor,
          typename Compare = std::less<Key>,
          typename Traits = btree_default_traits<Key, Value>,
          typename Allocator = std::allocator<Value>>
class btree {
private:
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
        node* n = self.root;
        if (!n) {
            return self.end();
        }
        while (!n->is_leafnode()) {
            const auto* inner = static_cast<const inner_node*>(n);
            slot_type slot = self.find_lower_bound(inner, key);
            n = inner->childs[slot];
        }
        auto* leaf = static_cast<leaf_node*>(n);
        slot_type slot = self.find_lower_bound(leaf, key);
        return Iterator(leaf, slot);
    }

    [[nodiscard]] iterator lower_bound(const key_type& key) { return lower_bound_impl<iterator>(*this, key); }
    [[nodiscard]] const_iterator lower_bound(const key_type& key) const {
        return lower_bound_impl<const_iterator>(*this, key);
    }

private:
    using level_type = size_type;
    using slot_type = size_type;

    static const slot_type leaf_slots_max = traits_type::leaf_slots;
    static const slot_type inner_slots_max = traits_type::inner_slots;
    static const slot_type leaf_slots_min = leaf_slots_max / 2;
    static const slot_type inner_slots_min = inner_slots_max / 2;

    struct node {
        slot_type level{};
        slot_type slot_count{};
        [[nodiscard]] bool is_leafnode() const noexcept { return level == 0; }
        [[nodiscard]] bool is_full() const noexcept { return slot_count == leaf_slots_max; }
        [[nodiscard]] bool is_few() const noexcept { return slot_count <= leaf_slots_min; }
        [[nodiscard]] bool is_underflow() const noexcept { return slot_count < leaf_slots_min; }
    };

    struct inner_node : public node {
        using alloc_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<inner_node>;

        std::array<key_type, inner_slots_max> keys;
        std::array<node*, inner_slots_max + 1> childs;

        [[nodiscard]] const key_type& key(slot_type slot) const noexcept { return keys[slot]; }
    };

    struct leaf_node : public node {
        using alloc_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<leaf_node>;

        leaf_node* previous_leaf{};
        leaf_node* next_leaf{};

        std::array<value_type, leaf_slots_max> data;

        [[nodiscard]] const key_type& key(slot_type slot) const { return key_extractor(data[slot]); }
        void set_slot(slot_type slot, const value_type& value) noexcept { data[slot] = value; }
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

    [[nodiscard]] leaf_node* allocate_leaf() {
        auto* leaf = allocate_from_allocator<leaf_node>(allocator);
        ++stats.leaves;
        return leaf;
    }

    [[nodiscard]] inner_node* allocate_inner(slot_type level) {
        auto* inner = allocate_from_allocator<inner_node>(allocator, level);
        ++stats.inner_nodes;
        return inner;
    }

    void deallocate_node(node* n) {
        if (n->is_leafnode()) {
            deallocate_from_allocator(allocator, static_cast<leaf_node*>(n));
            --stats.leaves;
        } else {
            deallocate_from_allocator(allocator, static_cast<inner_node*>(n));
            --stats.inner_nodes;
        }
    }

    void clear_recursive(node* n) {
        BPLUSTREE_ASSERT(n != nullptr);
        if (n->is_leafnode()) {
            // data objects are deleted by leaf_node's destructor
        } else {
            inner_node* inner = static_cast<inner_node*>(n);
            for (slot_type slot = 0; slot < inner->slot_count + 1; ++slot) {
                clear_recursive(inner->childs[slot]);
                deallocate_node(inner->childs[slot]);
            }
        }
    }

    template <typename Node, typename Comparator>
    [[nodiscard]] slot_type find_key(const Node* n, const Comparator& comp, const key_type& key) const noexcept {
        if (n->slot_count == 0) {
            return 0;
        }
        slot_type lo = 0, hi = n->slot_count;
        while (lo < hi) {
            slot_type mid = (lo + hi) / 2;
            if (comp(key_compare, n->key(mid), key)) {
                hi = mid;
            } else {
                lo = mid + 1;
            }
        }
        return lo;
    }

    template <typename Node>
    [[nodiscard]] slot_type find_lower_bound(const Node* n, const key_type& key) const noexcept {
        return find_key(n, details::greater_than_or_equal_to, key);
    }

    template <typename Node>
    [[nodiscard]] slot_type find_upper_bound(const Node* n, const key_type& key) const noexcept {
        return find_key(n, details::greater_than, key);
    }

    node* root{};
    leaf_node* head_leaf{};
    leaf_node* tail_leaf{};
    tree_stats stats;
    key_extractor_type key_extractor;
    key_compare_type key_compare;
    allocator_type allocator;

    template <typename V>
    class iterator_base {
    public:
        using key_type = Key;
        using value_type = std::decay_t<V>;
        using reference = V&;
        using pointer = V*;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;

        using leaf_node_type = std::conditional_t<std::is_const_v<V>, const leaf_node, leaf_node>;

        iterator_base() noexcept = default;
        iterator_base(leaf_node_type* l, slot_type s) noexcept : current_leaf{l}, current_slot{s} {}

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

    private:
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

        leaf_node_type* current_leaf{};
        slot_type current_slot{};
    };
};
