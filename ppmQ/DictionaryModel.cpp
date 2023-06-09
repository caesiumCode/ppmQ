#include "DictionaryModel.hpp"

DictionaryModel::DictionaryModel()
{
    reset();
}

void DictionaryModel::reset()
{
    for (uint64_t k = 0; k < 256; k++) frequency[k] = 1;
    
    total = 256;
    finger = 0;
}

void DictionaryModel::disp()
{
    for (std::size_t i = 0; i < 256; i++)
    {
        std::cout << std::setw(3) << (reinterpret_cast<char&>(i) == '\n' ? 'N' : reinterpret_cast<char&>(i));
        std::cout << std::setw(15) << frequency[i];
        std::cout << std::endl;
    }
}

uint64_t DictionaryModel::frq(uint8_t byte)
{
    return frequency[byte];
}

uint64_t DictionaryModel::cdf(uint8_t byte)
{
    uint64_t c = 0;
    
    for (uint8_t ch = 0; ch != byte; ch++) c += frequency[ch];
    
    return c;
}

uint64_t DictionaryModel::sum()
{
    return total;
}

void DictionaryModel::init_search()
{
    finger = 0;
}

bool DictionaryModel::next(uint8_t& byte, uint64_t& f)
{
    if (finger == 256) return false;
    
    byte = finger;
    f    = frequency[byte];
    
    finger++;
    return true;
}

void DictionaryModel::set_escape(bool mode)
{
    
}

void DictionaryModel::update(uint8_t byte)
{
    frequency[byte]++;
    total++;
}
