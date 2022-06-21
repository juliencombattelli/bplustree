#define private public
#include <bplustree.hpp>
#undef private

#include <map>
#include <string>
#include <vector>

int main() {
    using btree = btree<int, std::string, std::hash<std::string>>;
    btree bt;
    (void)bt.lower_bound(42);
    // (void)bt.upper_bound(42);
    /*{
        btree::iterator it;
        ++it;
        (void)it++;
        --it;
        (void)it--;
    }
    {
        btree::const_iterator it;
        ++it;
        (void)it++;
        --it;
        (void)it--;
    }
    {
        btree::reverse_iterator it;
        ++it;
        (void)it++;
        --it;
        (void)it--;
    }
    {
        btree::const_reverse_iterator it;
        ++it;
        (void)it++;
        --it;
        (void)it--;
    }*/
}
