#include "PPMModel.hpp"

PPMModel::PPMModel()
{
    ORDER = 0;
    reset();
    
    for (std::size_t i = 0; i < 64; i++) lc_value[i] = uint64_t(1) << i;
    
    rng_mask[0] = 0;
    rng_mask[1] = 1;
    for (std::size_t i = 2; i < 64; i++) rng_mask[i] = rng_mask[i-1] + lc_value[i];
}

void PPMModel::set_order(uint8_t order)
{
    ORDER = order;
}


void PPMModel::reset()
{
    // Memory
    memory.clear();
    memory.push_back(std::vector<ContextNode>(CHUNCK_SIZE));
    next_free = 1;
    
    root = &memory[0][0];
    
    hand    = {root};
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
    else if (byte == 0)
    {
        uint8_t      length = 0;
        ContextNode* node   = Idx2Ptr(hand[finger]->child);
        
        s.cdf = 0;
        while (node != root)
        {
            s.cdf += inclusion[node->symbol] * Rnk2Val(node->count);
            length++;
            
            node = Idx2Ptr(node->brother);
        }
        
        s.count = length;
        s.total = s.cdf + s.count;
    }
    else
    {
        uint8_t      length = 0;
        ContextNode* node   = Idx2Ptr(hand[finger]->child);
        
        s.cdf   = 0;
        s.count = 0;
        
        // cdf
        while (node != root && node->symbol != byte)
        {
            s.cdf += inclusion[node->symbol] * Rnk2Val(node->count);
            length++;
            
            node = Idx2Ptr(node->brother);
        }
        
        // freq
        if (node != root) s.count = Rnk2Val(node->count);
        
        // total
        s.total = s.cdf;
        while (node != root)
        {
            s.total += inclusion[node->symbol] * Rnk2Val(node->count);
            length++;
            
            node = Idx2Ptr(node->brother);
        }
        
        s.total += length;
        if (s.count == 0) s.cdf = s.total;
    }
}

uint64_t PPMModel::sum()
{
    if (finger == -1) return 256;
    
    uint8_t length    = 0;
    ContextNode* node = Idx2Ptr(hand[finger]->child);
    
    uint64_t c = 0;
    
    while (node != root)
    {
        c += inclusion[node->symbol] * Rnk2Val(node->count);
        length++;
        
        node = Idx2Ptr(node->brother);
    }
    
    return c + length;
}

void PPMModel::init_search()
{
    if (finger >= 0) context_finger = hand[finger]->child;
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
            if (context_finger == 0)
            {
                ContextNode* node = Idx2Ptr(hand[finger]->child);
                
                byte = 0;
                f    = 0;
                
                while (node != root)
                {
                    f++;
                    node = Idx2Ptr(node->brother);
                }
            }
            else
            {
                ContextNode* node = Idx2Ptr(context_finger);
                
                byte = node->symbol;
                f    = Rnk2Val(node->count);
                
                context_finger = Idx2Ptr(context_finger)->brother;
            }
        }
        while(!inclusion[byte]);
    }
    
    return true;
}

void PPMModel::set_escape(bool mode)
{
    if (mode)
    {
        for (ContextNode* node = Idx2Ptr(hand[finger]->child); node != root; node = Idx2Ptr(node->brother)) inclusion[node->symbol] = false;
        
        finger--;
        
        if (finger >= 0) context_finger = hand[finger]->child;
        else             null_finger    = 0;
    }
    else if (!mode)
    {
        finger = hand.size()-1;
        while (finger > 0 && hand[finger]->child == 0) finger--;
        
        context_finger = hand[finger]->child;
        
        std::fill(inclusion.begin(), inclusion.end(), true);
    }
}

void PPMModel::update(uint8_t byte)
{
    if (byte != 0)
    {
        std::vector<ContextIdx> indexes(ORDER+1, 0);
        increment_counter(hand[0]->count);
        //hand[0]->count++;
        
        for (std::size_t k = 0; k < hand.size(); k++)
        {
            ContextIdx idx_rev = 0;
            ContextIdx idx     = hand[k]->child;
            
            while (idx != 0 && Idx2Ptr(idx)->symbol != byte)
            {
                idx_rev = idx;
                idx = Idx2Ptr(idx)->brother;
            }
            
            if (idx != 0)
            {
                increment_counter(Idx2Ptr(idx)->count);
                //Idx2Ptr(idx)->count++;
                
                if (idx != hand[k]->child && idx_rev != 0)
                {
                    Idx2Ptr(idx_rev)->brother = Idx2Ptr(idx)->brother;
                    Idx2Ptr(idx)->brother     = hand[k]->child;
                    
                    hand[k]->child = idx;
                }
            }
            else
            {
                ContextIdx new_idx = (uint32_t(memory.size()-1) * CHUNCK_OFFSET) + next_free;
                ContextNode* new_node = Idx2Ptr(new_idx);
                new_node->symbol    = byte;
                new_node->count     = lc_offset;
                new_node->brother   = hand[k]->child;
                
                hand[k]->child = new_idx;
                
                if (next_free == CHUNCK_SIZE-1)
                {
                    memory.push_back(std::vector<ContextNode>(CHUNCK_SIZE));
                    next_free = 0;
                }
                else next_free++;
            }
        }
        
        if (hand.size() <= ORDER) hand.push_back(nullptr);
        for (std::size_t k = hand.size()-1; k > 0; k--) hand[k] = Idx2Ptr(hand[k-1]->child);
    }
}

uint64_t PPMModel::Rnk2Val(uint8_t rank)
{
    return lc_value[lc_pointer[rank]];
}

void PPMModel::increment_counter(uint8_t& rank)
{
    if ((rng() & rng_mask[rank >= lc_offset ? rank - lc_offset : 0 ]) == 0) rank++;
}

uint64_t PPMModel::rng()
{
    uint64_t z = (rng_state += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

ContextNode* PPMModel::Idx2Ptr(ContextIdx idx)
{
    return &memory[idx >> 16][idx & COORD_MASK];
}
