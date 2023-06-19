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

using ContextIdx = uint32_t;

struct ContextNode
{
    ContextNode()
    {
        symbol   = 0;
        count    = 0;
        
        brother  = 0;
        
        child    = 0;
    }
    
    uint8_t symbol;
    uint8_t count;
    //uint32_t count;
    
    ContextIdx brother;
    
    ContextIdx child;
};

class PPMModel
{
public:
    PPMModel();
    
    void set_order(uint8_t order);
    
    void reset();
    
    void     stat(uint8_t byte, Statistic& s);
    uint64_t sum();
    
    void init_search();
    bool next(uint8_t& byte, uint64_t& f);
    void set_escape(bool mode);
    
    void update(uint8_t byte);
    
private: // Context Trie Model
    uint8_t ORDER;
    
    ContextNode* root;
    std::array<bool, 256> inclusion;
    
private: // Search shortcut
    std::vector<ContextNode*> hand;
    int8_t   finger;
    uint16_t null_finger;
    
    ContextIdx context_finger;
    
private: // Log Counter
    std::array<uint64_t, 64>    lc_value;
    std::array<std::size_t, 64> lc_pointer;
    std::size_t                 lc_offset;
    
    uint64_t    Rnk2Val(uint8_t rank);
    void        increment_counter(uint8_t& rank);
    
private: // Fast Pseudo-Random Generator (https://prng.di.unimi.it/splitmix64.c)
    std::array<uint64_t, 64> rng_mask;
    uint64_t                 rng_state;
    
    uint64_t rng();
    
private: // memory management
    static const std::size_t CHUNCK_SIZE    = std::size_t(1) << 16;
    static const uint32_t    CHUNCK_OFFSET  = uint32_t(1) << 16;
    static const uint32_t    COORD_MASK     = (uint32_t(1) << 16) - 1;
    
    uint32_t next_free;
    std::vector<std::vector<ContextNode>> memory;
    
    ContextNode* Idx2Ptr(ContextIdx idx);
};

#endif /* PPMModel_hpp */
