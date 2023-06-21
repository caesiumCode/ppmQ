#include "PPMModel.hpp"

PPMModel::PPMModel()
{
    ORDER = 0;
    MAX_CONTEXT = std::numeric_limits<uint32_t>::max();
    reset();
    
    double sqrt2 = std::sqrt(2);
    double sqrt_sqrt2 = std::sqrt(sqrt2);
    lc_value[0] = 0;
    
    std::size_t i = 1;
    int ith = 0;
    double exact_value = 0;
    while (i < LC_SLOTS)
    {
        int k = ith/4;
        uint64_t power_of_2 = 1ull << k;
        
        switch (ith%4)
        {
            case 0:
                exact_value = power_of_2;
                break;
                
            case 1:
                exact_value = sqrt_sqrt2 * double(power_of_2);
                break;
                
            case 2:
                exact_value = sqrt2 * double(power_of_2);
                break;
                
            case 3:
                exact_value = sqrt2 * sqrt_sqrt2 * double(power_of_2);
                break;
                
            default:
                break;
        }
        
        lc_value[i] = std::round(exact_value);
        
        if (lc_value[i] < lc_value[i-1]) break; // Happens if out of range
        
        if (lc_value[i] > lc_value[i-1]) i++;
        ith++;
    }
    
    for (std::size_t j = i; j < LC_SLOTS; j++) lc_value[j] = std::numeric_limits<uint64_t>::max();
}

void PPMModel::set_order(uint8_t order)
{
    ORDER = order;
}

void PPMModel::set_max_context(uint32_t max_context)
{
    MAX_CONTEXT = max_context;
}

void PPMModel::disp()
{
    std::size_t i_max = 0;
    for (std::size_t i = 0; i < LC_SLOTS; i++) if (lc_population[lc_pointer[i]] > 0) i_max = i;
    
    std::cout << "memory size " << memory_size << std::endl;
    std::cout << "lc_offset " << lc_offset << std::endl;
    for (std::size_t i = 0; i <= i_max; i++)
    {
        std::cout << std::setw(4) << lc_pointer[i];
        std::cout << std::setw(15) << lc_value[lc_pointer[i]];
        std::cout << std::setw(15) << lc_population[lc_pointer[i]];
        std::cout << std::endl;
    }
}

void PPMModel::disp_trie()
{
    std::string stack;
    disp_trie(stack, 0, 0);
    std::cout << std::endl;
}

void PPMModel::disp_trie(std::string& stack, int depth, ContextIdx idx)
{
    ContextNode* node = Idx2Ptr(idx);
    
    stack.push_back(node->symbol == '\n' ? 'N' : node->symbol);
    std::cout << std::string(3*depth, ' ');
    std::cout << "(" << idx << ")" << stack << (node->big_brother ? "*" : "") << " [" << int(lc_value[lc_pointer[node->count]]) << "] ";
    
    ContextIdx child = node->child;
    while (child != idx)
    {
        std::cout << child << " ";
        child = Idx2Ptr(child)->brother;
    }
    std::cout << std::endl;
    
    child = node->child;
    while (child != idx)
    {
        disp_trie(stack, depth+1, child);
        child = Idx2Ptr(child)->brother;
    }
    
    stack.pop_back();
}

std::string PPMModel::get_parameters()
{
    return std::to_string(ORDER) + "," + std::to_string(MAX_CONTEXT) + "," + std::to_string(memory_size);
}

std::string PPMModel::get_order()
{
    return std::to_string(ORDER);
}


void PPMModel::reset()
{
    // Memory
    memory.clear();
    memory.push_back(std::vector<ContextNode>(CHUNCK_SIZE));
    next_free = 1;
    checkpoint = 0;
    memory_size = 1;
    
    root = &memory[0][0];
    root->count   = 0;
    root->brother = 0;
    root->child   = 0;
    
    hand     = {root};
    hand_idx = {0};
    finger   = -1;
    
    // highly specialised variables
    null_finger = 0;
    lc_offset   = 0;
    for (std::size_t i = 0; i < LC_SLOTS; i++) lc_pointer[i] = i;
    for (std::size_t i = 0; i < LC_SLOTS; i++) lc_population[i] = 0;
    lc_population[0] = 1;
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
        
        if (node == hand[finger])
        {
            s.cdf   = 0;
            s.count = 1;
            s.total = 1;
            return;
        }
        
        s.cdf = 0;
        while (node != hand[finger])
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
        while (node != hand[finger] && node->symbol != byte)
        {
            s.cdf += inclusion[node->symbol] * Rnk2Val(node->count);
            length++;
            
            node = Idx2Ptr(node->brother);
        }
        
        // freq
        if (node != hand[finger]) s.count = Rnk2Val(node->count);
        
        // total
        s.total = s.cdf;
        while (node != hand[finger])
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
    
    if (node == hand[finger]) return 1;
    
    uint64_t c = 0;
    
    while (node != hand[finger])
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
            if (context_finger == hand_idx[finger])
            {
                ContextNode* node = Idx2Ptr(hand[finger]->child);
                
                byte = 0;
                f    = 0;
                
                while (node != hand[finger])
                {
                    f++;
                    node = Idx2Ptr(node->brother);
                }
                
                f = f == 0 ? 1 : f;
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
        for (ContextNode* node = Idx2Ptr(hand[finger]->child); node != hand[finger]; node = Idx2Ptr(node->brother)) inclusion[node->symbol] = false;
        
        finger--;
        
        if (finger >= 0) context_finger = hand[finger]->child;
        else             null_finger    = 0;
    }
    else
    {
        finger = hand.size()-1;
        while (finger > 0 && hand[finger]->child == hand_idx[finger]) finger--;
        
        context_finger = hand[finger]->child;
        
        std::fill(inclusion.begin(), inclusion.end(), true);
    }
}

void PPMModel::update(uint8_t byte)
{
    if (byte != 0)
    {
        increment_counter(hand[0]->count);
        
        for (std::size_t k = 0; k < hand.size(); k++)
        {
            ContextNode* node_prev = hand[k];
            ContextNode* node      = Idx2Ptr(hand[k]->child);
            
            while (node != hand[k] && node->symbol != byte)
            {
                node_prev = node;
                node      = Idx2Ptr(node->brother);
            }
            
            // Move-to-front if symbol found
            if (node != hand[k])
            {
                increment_counter(node->count, hand[k]->count > node->count);
                
                if (node != Idx2Ptr(hand[k]->child))
                {
                    ContextIdx tmp = node_prev->brother;
                    
                    node_prev->brother     = node->brother;
                    node_prev->big_brother = node->big_brother;
                
                    node->brother      = hand[k]->child;
                    node->big_brother  = false;
                    
                    hand[k]->child = tmp;
                }
            }
            // Insert new context if symbol not found
            else
            {
                ContextIdx new_idx = new_context();
                ContextNode* new_node = Idx2Ptr(new_idx);
                
                new_node->symbol      = byte;
                new_node->count       = lc_offset + 1;
                new_node->brother     = hand[k]->child;
                new_node->child       = new_idx;
                new_node->big_brother = (hand[k]->child == hand_idx[k]);
                
                hand[k]->child = new_idx;
                
                lc_population[1]++;
            }
        }
        
        if (hand.size() <= ORDER)
        {
            hand.push_back(nullptr);
            hand_idx.push_back(0);
        }
        for (std::size_t k = hand.size()-1; k > 0; k--)
        {
            hand_idx[k] = hand[k-1]->child;
            hand[k]     = Idx2Ptr(hand_idx[k]);
        }
        hand_idx[0] = 0;
        
        if (memory_size - lc_population[0] >= MAX_CONTEXT) shift_all();
    }
}

uint64_t PPMModel::Rnk2Val(uint8_t rank)
{
    return lc_value[lc_pointer[rank]] == 0 ? 1 : lc_value[lc_pointer[rank]];
}

void PPMModel::increment_counter(uint8_t& rank, bool flag)
{
    if (rank <= lc_offset) rank = lc_offset;
    
    if (rank < LC_SLOTS-1)
    {
        uint64_t range = lc_value[lc_pointer[rank+1]] - lc_value[lc_pointer[rank]];
        
        if (flag && rng() % range == 0)
        {
            lc_population[lc_pointer[rank]]--;
            rank = rank <= lc_offset ? lc_offset + 1 : rank + 1;
            lc_population[lc_pointer[rank]]++;
        }
    }
}

void PPMModel::shift_all()
{
    // Update population
    lc_population[0] += lc_population[1];
    for (std::size_t i = 1; i < LC_SLOTS-1; i++) lc_population[i] = lc_population[i+1];
    lc_population.back() = 0;
    
    // update pointers
    for (std::size_t i = LC_SLOTS-1; i > 0; i--) lc_pointer[i] = lc_pointer[i-1];
    
    lc_offset++;
    checkpoint = 0;
    
    if (lc_offset >= LC_SLOTS/2) delete_offset();
}

void PPMModel::delete_offset()
{
    for (std::size_t i = 0; i < LC_SLOTS; i++) lc_pointer[i] = i;
    for (std::size_t i = 0; i < memory.size()-1; i++) for (std::size_t j = 0; j < CHUNCK_SIZE; j++) memory[i][j].count = memory[i][j].count <= lc_offset ? 0 : memory[i][j].count - lc_offset;
    for (std::size_t j = 0; j < next_free; j++) memory.back()[j].count = memory.back()[j].count <= lc_offset ? 0 : memory.back()[j].count - lc_offset;
    
    lc_offset = 0;
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

ContextIdx PPMModel::next(ContextIdx idx)
{
    if ((idx & COORD_MASK) < CHUNCK_SIZE-1) return idx+1;
    return uint32_t((idx >> 16) + 1) * CHUNCK_OFFSET;
}

ContextIdx PPMModel::new_context()
{
    if (lc_population[0] > 0) while (true)
    {
        uint32_t chunck = checkpoint >> 16;
        uint32_t coord  = checkpoint & COORD_MASK;
        
        if ((chunck * CHUNCK_OFFSET) + coord >= memory_size)
        {
            checkpoint = 0;
            chunck = 0;
            coord = 0;
        }
        
        
        for (uint32_t i = chunck; i < memory.size(); i++)
            for (uint32_t j = (i == chunck ? coord : 0); j < (i == memory.size()-1 ? next_free : CHUNCK_SIZE); j++)
                if (lc_value[lc_pointer[memory[i][j].count]] == 0 && memory[i][j].child == (i*CHUNCK_SIZE+j) && is_out_of_hand(i*CHUNCK_SIZE+j))
        {
            ContextIdx idx = (i * CHUNCK_OFFSET) + j;
            checkpoint = next(idx);
            free_context(idx);
            lc_population[0]--;
            return idx;
        }
        
        if (checkpoint == 0) shift_all();
        checkpoint = 0;
    }
    else
    {
        ContextIdx new_idx = (uint32_t(memory.size()-1) * CHUNCK_OFFSET) + next_free;
        
        if (next_free == CHUNCK_SIZE-1)
        {
            memory.push_back(std::vector<ContextNode>(CHUNCK_SIZE));
            next_free = 0;
        }
        else next_free++;
        
        memory_size++;
        
        return new_idx;
    }
}

void PPMModel::free_context(ContextIdx idx)
{
    ContextNode* node = Idx2Ptr(idx);
    
    // Already freed
    if (node->brother == idx) return;
    
    // Free children
    ContextIdx child = node->child;
    while (child != idx)
    {
        ContextIdx next_child = Idx2Ptr(child)->brother;
        
        free_context_deep(child);
        
        child = next_child;
    }
    node->child = idx;
        
    // Destroy connection
    ContextNode* previous = node;
    bool parent = false;
    while ((!parent && previous->brother != idx) || (parent && previous->child != idx))
    {
        if (parent)
        {
            parent = false;
            previous = Idx2Ptr(previous->child);
        }
        else
        {
            parent = previous->big_brother;
            previous = Idx2Ptr(previous->brother);
        }
    }
    
    if (parent) previous->child = node->brother;
    else
    {
        previous->brother     = node->brother;
        previous->big_brother = node->big_brother;
    }
    
    // Free current node
    node->brother = idx;
    node->child   = idx;
    lc_population[lc_pointer[node->count]]--;
    node->count = lc_offset;
    lc_population[0]++;
}

void PPMModel::free_context_deep(ContextIdx idx)
{
    ContextNode* node = Idx2Ptr(idx);
    
    // Already freed
    if (Idx2Ptr(node->brother) == node) return;
    
    // Free children
    ContextIdx child = node->child;
    while (child != idx)
    {
        ContextIdx next_child = Idx2Ptr(child)->brother;
        
        free_context_deep(child);
        
        child = next_child;
    }
    
    // free current node
    node->brother = idx;
    node->child   = idx;
    lc_population[lc_pointer[node->count]]--;
    node->count = lc_offset;
    lc_population[0]++;
}

bool PPMModel::is_out_of_hand(ContextIdx idx)
{
    for (std::size_t i = 0; i < hand_idx.size(); i++) if (hand_idx[i] == idx) return false;
    return true;
}
