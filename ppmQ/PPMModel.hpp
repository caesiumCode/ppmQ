#ifndef PPMModel_hpp
#define PPMModel_hpp

#include "Model.hpp"

#include <unordered_map>
#include <string>
#include <iostream>
#include <vector>
#include <utility>
#include <iomanip>

struct ContextNode
{
    ContextNode()
    {
        symbol   = '\0';
        total    = 1;
        children = {{0, {1, nullptr}}};
    }
    ~ContextNode()
    {
        for (const auto&[s, child] : children) if (s != 0) delete child.second;
    }
    
    void print(std::string& stack)
    {
        stack.push_back(reinterpret_cast<char&>(symbol));
        std::cout << stack << std::endl;
        std::cout << "| total=" << total << std::endl;
        for (const auto&[s, child] : children)
        {
            uint8_t ss = s;
            char ch = reinterpret_cast<char&>(ss);
            std::cout << "| " << (ch == '\n' ? 'N' : ch) << std::setw(13) << child.first << std::endl;
        }
        std::cout << std::endl;
        for (const auto&[s, child] : children)
        {
            if (s != 0) child.second->print(stack);
        }
        stack.pop_back();
    }
    
    uint8_t  symbol;
    uint64_t total;
    
    std::unordered_map<uint8_t, std::pair<uint64_t, ContextNode*>> children;
};

using map_iterator = std::unordered_map<uint8_t, std::pair<uint64_t, ContextNode*>>::iterator;

class PPMModel : public Model
{
public:
    PPMModel();
    ~PPMModel();
    
    void set_order(uint8_t order);
    
    void reset();
    void disp();
    
    uint64_t frq(uint8_t byte);
    uint64_t cdf(uint8_t byte);
    uint64_t sum();
    
    void init_search();
    bool next(uint8_t& byte, uint64_t& f);
    void set_escape(bool mode);
    
    void update(uint8_t byte);
    
private:
    uint8_t ORDER;
    
    ContextNode* root;
    
    std::vector<ContextNode*>   hand;
    int8_t                      finger;
    
private:
    uint16_t     null_finger;
    map_iterator context_finger;
};

#endif /* PPMModel_hpp */
