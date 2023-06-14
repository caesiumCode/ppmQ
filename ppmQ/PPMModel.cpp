#include "PPMModel.hpp"

PPMModel::PPMModel()
{
    ORDER = 0;
    root = nullptr;
    reset();
    
    for (std::size_t i = 0; i < 64; i++) lc_value[i] = uint64_t(1) << i;
    
    rng_mask[0] = 0;
    rng_mask[1] = 1;
    for (std::size_t i = 2; i < 64; i++) rng_mask[i] = rng_mask[i-1] + lc_value[i];
}

PPMModel::~PPMModel()
{
    delete root;
}

void PPMModel::set_order(uint8_t order)
{
    ORDER = order;
    hand.resize(ORDER, nullptr);
}


void PPMModel::reset()
{
    delete root;
    
    root = new ContextNode;
    
    hand    = std::vector<ContextNode*>(ORDER+1, nullptr);
    hand[0] = root;
    finger  = -1;
    
    // highly specialised variables
    null_finger = 0;
    lc_offset   = 0;
    for (std::size_t i = 0; i < 64; i++) lc_pointer[i] = i;
    rng_state   = 0;
}

void PPMModel::stat(uint8_t byte, Statistic& s)
{
    if (finger == -1)
    {
        s.cdf   = byte;
        s.count = 1;
        s.total = 256;
    }
    else
    {
        std::vector<ContextNode*>& set = hand[finger]->children;
        std::size_t i   = 0;
        
        s.cdf   = 0;
        s.count = 0;
        s.total = set.size();
        bool new_sym = false;
        
        for (i = 0; i < set.size() && set[i]->symbol != byte; i++) if (!exclusion[set[i]->symbol]) s.cdf += set[i]->count;// lc_value[lc_pointer[set[i]->count]];
        
        if (i < set.size()) s.count = set[i]->count;// lc_value[lc_pointer[set[i]->count]];
        else if (byte == 0) s.count = set.size();
        else                new_sym = true;
        
        s.total += s.cdf;
        for (; i < set.size(); i++) if (!exclusion[set[i]->symbol]) s.total += set[i]->count;// lc_value[lc_pointer[set[i]->count]];
        
        if (new_sym)
        {
            s.cdf   = s.total;
            s.count = 0;
        }
    }
}

void PPMModel::stat_escape(Statistic& s)
{
    s.count = hand[finger]->children.size();
    s.cdf  -= hand[finger]->children.size();
    
}

uint64_t PPMModel::frq(uint8_t byte)
{
    if (finger == -1)   return 1;
    if (byte == 0)      return hand[finger]->children.size();
    
    for (const auto& child : hand[finger]->children) if (child->symbol == byte) return child->count;// lc_value[lc_pointer[child->count]];
    return 0;
}

uint64_t PPMModel::sum()
{
    if (finger == -1) return 256;
    
    uint64_t c = hand[finger]->children.size();
    for (const auto& child : hand[finger]->children) if (!exclusion[child->symbol]) c += child->count;// lc_value[lc_pointer[child->count]];
    return c;
}

void PPMModel::init_search()
{
    if (finger >= 0) context_finger = 0;
    else             null_finger    = 0;
}

bool PPMModel::next(uint8_t& byte, uint64_t& f)
{
    if (finger == -1)
    {
        if (null_finger == 256) return false;
        
        byte = null_finger;
        f    = 1;
        
        null_finger++;
    }
    else
    {
        do
        {
            if (context_finger == hand[finger]->children.size())
            {
                byte = 0;
                f    = hand[finger]->children.size();
            }
            else
            {
                const ContextNode* node = hand[finger]->children[context_finger];
                
                byte = node->symbol;
                f    = node->count;//lc_value[lc_pointer[node->count]];
                
                context_finger++;
            }
        }
        while(exclusion[byte]);
    }
    
    return true;
}

void PPMModel::set_escape(bool mode)
{
    if (mode)
    {
        for (const auto& child : hand[finger]->children) exclusion[child->symbol] = true;
        
        finger--;
        
        if (finger >= 0) context_finger = 0;
        else             null_finger    = 0;
    }
    else if (!mode)
    {
        finger = ORDER;
        while (hand[finger] == nullptr)                      finger--;
        while (finger > 0 && hand[finger]->children.empty()) finger--;
        
        context_finger = 0;
        
        std::fill(exclusion.begin(), exclusion.end(), false);
    }
}

void PPMModel::update(uint8_t byte)
{
    if (byte != 0)
    {
        std::vector<std::size_t> indexes(ORDER+1, 0);
        //increment_counter(hand[0]->count);
        hand[0]->count++;
        
        for (std::size_t k = 0; k <= ORDER; k++) if (hand[k])
        {
            std::size_t set_size = hand[k]->children.size();
            std::size_t& i       = indexes[k];
            while (i < set_size && hand[k]->children[i]->symbol != byte) i++;
            
            if (i < set_size) hand[k]->children[i]->count++;// increment_counter(hand[k]->children[i]->count);
            else
            {
                ContextNode* new_symbol = new ContextNode;
                new_symbol->symbol      = byte;
                new_symbol->count       = 1;// lc_offset;
                new_symbol->children    = {};
                
                hand[k]->children.push_back(new_symbol);
            }
            
            while (i > 0 && hand[k]->children[i]->count >= hand[k]->children[i-1]->count)
            {
                std::swap(hand[k]->children[i-1], hand[k]->children[i]);
                i--;
            }
        }
        
        for (std::size_t k = ORDER; k > 0; k--) if (hand[k-1]) hand[k] = hand[k-1]->children[indexes[k-1]];
    }
}

void PPMModel::increment_counter(uint8_t& counter)
{
    if ((rng() & rng_mask[counter >= lc_offset ? counter - lc_offset : 0 ]) == 0) counter++;
}

uint64_t PPMModel::rng()
{
    uint64_t z = (rng_state += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}
