#ifndef PPMModel_hpp
#define PPMModel_hpp

#include <unordered_set>
#include <string>
#include <iostream>
#include <vector>
#include <utility>
#include <iomanip>

struct Statistic
{
    uint64_t cdf;
    uint64_t count;
    uint64_t total;
};

struct ContextNode
{
    ContextNode()
    {
        symbol   = 0;
        count    = 0;
        children = {};
    }
    ~ContextNode()
    {
        for (const auto& child : children) delete child;
    }
    
    uint8_t symbol;
    //uint8_t count;
    uint32_t count;
    
    std::vector<ContextNode*> children;
};

using map_iterator = std::unordered_map<uint8_t, ContextNode*>::iterator;

class PPMModel
{
public:
    PPMModel();
    ~PPMModel();
    
    void set_order(uint8_t order);
    
    void reset();
    
    void     stat(uint8_t byte, Statistic& s);
    void     stat_escape(Statistic& s);
    uint64_t frq(uint8_t byte);
    uint64_t sum();
    
    void init_search();
    bool next(uint8_t& byte, uint64_t& f);
    void set_escape(bool mode);
    
    void update(uint8_t byte);
    
private: // Context Trie Model
    uint8_t ORDER;
    
    ContextNode* root;
    std::array<bool, 256> exclusion;
    
private: // Search shortcut
    std::vector<ContextNode*> hand;
    int8_t   finger;
    uint16_t null_finger;
    uint16_t context_finger;
    
private: // Log Counter
    std::array<uint64_t, 64>    lc_value;
    std::array<std::size_t, 64> lc_pointer;
    std::size_t                 lc_offset;
    
    void increment_counter(uint8_t& counter);
    
private: // Fast Pseudo-Random Generator (https://prng.di.unimi.it/splitmix64.c)
    std::array<uint64_t, 64> rng_mask;
    uint64_t                 rng_state;
    
    uint64_t rng();
    
private: // memory management
    static const std::size_t CHUNCK_SIZE = std::size_t(1) << 16;
    
    std::vector<std::array<ContextNode, CHUNCK_SIZE>> memory;
};

#endif /* PPMModel_hpp */
