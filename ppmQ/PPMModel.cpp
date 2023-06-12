#include "PPMModel.hpp"

PPMModel::PPMModel()
{
    ORDER = 0;
    root = nullptr;
    reset();
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
    finger  = 0;
    
    // highly specialised variables
    null_finger = 0;
}

void PPMModel::disp()
{
    std::string stack;
    root->print(stack);
}

uint64_t PPMModel::frq(uint8_t byte)
{
    if (finger == -1) return 1;
    
    map_iterator loc = hand[finger]->children.find(byte);
    
    if (loc == hand[finger]->children.end()) return 0;
    else                                     return loc->second.first;
}

uint64_t PPMModel::cdf(uint8_t byte)
{
    if (finger == -1) return byte;
    
    map_iterator loc = hand[finger]->children.find(byte);
    if (loc == hand[finger]->children.end()) return hand[finger]->total;
    
    uint64_t c = 0;
    for (map_iterator it = hand[finger]->children.begin(); it->first != byte; it++) c += it->second.first;
    return c;
}

uint64_t PPMModel::sum()
{
    return finger == -1 ? 256 : hand[finger]->total;
}

void PPMModel::init_search()
{
    if (finger >= 0) context_finger = hand[finger]->children.begin();
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
        if (context_finger == hand[finger]->children.end()) return false;
        
        byte = context_finger->first;
        f    = context_finger->second.first;
        
        context_finger++;
    }
    
    return true;
}

void PPMModel::set_escape(bool mode)
{
    if (mode)
    {
        finger--;
        
        if (finger >= 0) context_finger = hand[finger]->children.begin();
        else             null_finger    = 0;
    }
    else if (!mode)
    {
        finger = ORDER;
        while (hand[finger] == nullptr) finger--;
        
        context_finger = hand[finger]->children.begin();
    }
}

void PPMModel::update(uint8_t byte)
{
    if (byte == 0)
    {
        hand[finger]->children[0].first++;
        hand[finger]->total++;
        
    }
    else
    {
        for (std::size_t k = 0; k <= ORDER; k++) if (hand[k])
        {
            if (hand[k]->children.contains(byte))
            {
                hand[k]->children[byte].first++;
                hand[k]->total++;
            }
            else
            {
                ContextNode* new_symbol = new ContextNode;
                new_symbol->symbol      = byte;
                new_symbol->total       = 1;
                new_symbol->children[0] = {1, nullptr};
                
                hand[k]->children[byte] = {1, new_symbol};
                hand[k]->total++;
            }
        }
        
        for (std::size_t k = ORDER; k > 0; k--) if (hand[k-1]) hand[k] = hand[k-1]->children[byte].second;
    }
}
