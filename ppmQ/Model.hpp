#ifndef Model_hpp
#define Model_hpp

#include <stdint.h>

class Model
{
public:
    Model() = default;
    
    virtual void reset() = 0;
    virtual void disp() = 0;
    
    virtual uint64_t frq(uint8_t byte) = 0;
    virtual uint64_t cdf(uint8_t byte) = 0;
    virtual uint64_t sum() = 0;
    
    virtual void init_search() = 0;
    virtual bool next(uint8_t& byte, uint64_t& f) = 0;
    virtual void set_escape(bool mode) = 0;
    
    virtual void update(uint8_t byte) = 0;
};

#endif /* Model_hpp */
