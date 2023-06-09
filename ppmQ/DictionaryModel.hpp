#ifndef DictionaryModel_hpp
#define DictionaryModel_hpp

#include <array>
#include <iostream>
#include <iomanip>
#include <unordered_map>

#include "Model.hpp"


class DictionaryModel : public Model
{
public:
    DictionaryModel();
    
    void reset();
    
    uint64_t frq(uint8_t byte);
    uint64_t cdf(uint8_t byte);
    uint64_t sum();
    
    void init_search();
    bool next(uint8_t& byte, uint64_t& f);
    
    void update(uint8_t byte);
    
private:    
    std::array<uint64_t, 256> frequency;
    uint64_t total;
    
    std::size_t finger;
};

#endif /* DictionaryModel_hpp */
