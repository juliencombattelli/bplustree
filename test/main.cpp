#include <bplustree.hpp>

#include <string>
#include <vector>

int main() {
    using btree = btree<int, std::string, std::hash<std::string>>;
    btree bt;
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
