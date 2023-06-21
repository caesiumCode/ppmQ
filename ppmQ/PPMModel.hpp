#ifndef PPMModel_hpp
#define PPMModel_hpp

#include <limits>
#include <cmath>
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
        
        big_brother = false;
        brother     = 0;
        
        child    = 0;
    }
    
    uint8_t symbol;
    uint8_t count;
    
    bool big_brother;
    ContextIdx brother;
    
    ContextIdx child;
};

class PPMModel
{
public:
    PPMModel();
    
    void set_order(uint8_t order);
    void set_max_context(uint32_t max_context = std::numeric_limits<uint32_t>::max());
    void disp();
    void disp_trie();
    
    std::string get_order();
    std::string get_parameters();
    
    void reset();
    
    void     stat(uint8_t byte, Statistic& s);
    uint64_t sum();
    
    void init_search();
    bool next(uint8_t& byte, uint64_t& f);
    void set_escape(bool mode);
    
    void update(uint8_t byte);
    
private: // Context Trie Model
    uint8_t  ORDER;
    uint32_t MAX_CONTEXT;
    
    ContextNode* root;
    std::array<bool, 256> inclusion;
    
    void disp_trie(std::string& stack, int depth = 0, ContextIdx idx = 0);
    
private: // Search shortcut
    std::vector<ContextNode*> hand;
    std::vector<ContextIdx>   hand_idx;
    int8_t   finger;
    uint16_t null_finger;
    
    ContextIdx context_finger;
    
private: // Log Counter
    static const std::size_t LC_SLOTS = 256;
    
    std::array<uint64_t,    LC_SLOTS> lc_value;
    std::array<std::size_t, LC_SLOTS> lc_pointer;
    std::array<uint32_t,    LC_SLOTS> lc_population;
    std::size_t lc_offset;
    
    uint64_t    Rnk2Val(uint8_t rank);
    void        increment_counter(uint8_t& rank, bool flag = true);
    void        shift_all();
    void        delete_offset();
    
private: // Fast Pseudo-Random Generator (https://prng.di.unimi.it/splitmix64.c)
    uint64_t rng_state;
    
    uint64_t rng();
    
private: // memory management
    static const std::size_t CHUNCK_SIZE    = std::size_t(1) << 16;
    static const uint32_t    CHUNCK_OFFSET  = uint32_t(1) << 16;
    static const uint32_t    COORD_MASK     = (uint32_t(1) << 16) - 1;
    
    uint32_t memory_size;
    ContextIdx next_free;
    ContextIdx checkpoint;
    std::vector<std::vector<ContextNode>> memory;
    
    ContextNode* Idx2Ptr(ContextIdx idx);
    ContextIdx   next(ContextIdx idx);
    ContextIdx   new_context();
    bool         is_out_of_hand(ContextIdx idx);
    void         free_context(ContextIdx idx);
    void         free_context_deep(ContextIdx idx);
};

#endif /* PPMModel_hpp */
