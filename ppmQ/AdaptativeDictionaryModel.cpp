#include "AdaptativeDictionaryModel.hpp"

AdaptativeDictionaryModel::AdaptativeDictionaryModel()
{
    reset();
}

void AdaptativeDictionaryModel::reset()
{
    for (std::size_t& i : symbol_map) i = 256;
    symbol_map[0] = 0;
    
    symbol  = {0};
    counter = {1};
    total   = 1;
    
    finger = 0;
    escape = false;
}

void AdaptativeDictionaryModel::disp()
{
    for (std::size_t i = 0; i < symbol.size(); i++)
    {
        std::cout << std::setw(3) << (reinterpret_cast<char&>(symbol[i]) == '\n' ? 'N' : reinterpret_cast<char&>(symbol[i]));
        std::cout << std::setw(15) << counter[i];
        std::cout << std::endl;
    }
}

uint64_t AdaptativeDictionaryModel::frq(uint8_t byte)
{
    if (escape) return 1;
    
    if (symbol_map[byte] == 256)    return 0;
    else                            return counter[symbol_map[byte]];
}

uint64_t AdaptativeDictionaryModel::cdf(uint8_t byte)
{
    if (escape)                  return byte;
    if (symbol_map[byte] == 256) return total;
    
    uint64_t c = 0;
    
    for (std::size_t i = 0; i < symbol.size() && symbol[i] != byte; i++) c += counter[i];
    
    return c;
}

uint64_t AdaptativeDictionaryModel::sum()
{
    return escape ? 256 : total;
}

void AdaptativeDictionaryModel::init_search()
{
    finger = 0;
}

bool AdaptativeDictionaryModel::next(uint8_t& byte, uint64_t& f)
{
    if (escape)
    {
        if (finger == 256) return false;
        
        byte = finger;
        f    = 1;
    }
    else
    {
        if (finger == symbol.size()) return false;
        
        byte = symbol[finger];
        f    = counter[finger];
    }
    
    finger++;
    return true;
}

void AdaptativeDictionaryModel::set_escape(bool mode)
{
    escape = mode;
}

void AdaptativeDictionaryModel::update(uint8_t byte)
{
    if (!escape)
    {
        std::size_t i = symbol_map[byte];
        
        counter[i]++;
        
        while (i > 0 && counter[i] > counter[i-1])
        {
            symbol_map[byte]--;
            symbol_map[symbol[i-1]]++;
            
            std::swap(counter[i-1], counter[i]);
            std::swap(symbol[i-1], symbol[i]);
            
            i--;
        }
    }
    else
    {
        symbol_map[byte] = symbol.size();
        symbol.push_back(byte);
        counter.push_back(1);
    }
    total++;
}
