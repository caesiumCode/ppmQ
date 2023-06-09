#ifndef AdaptativeDictionaryModel_hpp
#define AdaptativeDictionaryModel_hpp

#include <array>
#include <iostream>
#include <iomanip>
#include <vector>

#include "Model.hpp"

class AdaptativeDictionaryModel : public Model
{
public:
    AdaptativeDictionaryModel();
    
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
    std::array<std::size_t, 256> symbol_map;
    
    std::vector<uint8_t>  symbol;
    std::vector<uint64_t> counter;
    uint64_t total;
    
    bool        escape;
    std::size_t finger;
};

#endif /* AdaptativeDictionaryModel_hpp */
