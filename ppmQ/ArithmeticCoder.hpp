#ifndef ArithmeticCoder_hpp
#define ArithmeticCoder_hpp

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>
#include <chrono>

#include "PPMModel.hpp"

using Timer         = std::chrono::high_resolution_clock;
using TimerMeasure  = std::chrono::time_point<Timer>;

class obstream
{
public:
    obstream(const std::string& filepath);
    void flush();
    
    obstream& operator<<(bool     bit);
    obstream& operator<<(uint64_t num);
    
private:
    const uint64_t BUFFER_SIZE = 1048576;
    
    std::ofstream fout;
    std::string buffer;
    
    std::size_t byte_pos;
    uint8_t     bit_pos;
    
private:
    void next_bit();
};

class ibstream
{
public:
    ibstream(const std::string& filepath, uint64_t offset = 0);
    bool        eof();    
    bool        read();
    
private:
    const uint64_t BUFFER_SIZE = 1048576;
    
    std::ifstream fin;
    std::string buffer;
    
    std::size_t byte_pos;
    uint8_t     bit_pos;
    
private:
    void next_bit();
};


class ArithmeticCoder
{
public:
    ArithmeticCoder(PPMModel& input_model);
    
    std::string get_encoding_filepath(const std::string& filepath);
    
    void encode(const std::string& filepath);
    void decode(const std::string& filepath);
    
private:
    PPMModel& model;
    
private:
    const uint64_t BUFFER_SIZE = 1048576;
    
    const uint8_t ESCAPE = 0;
    
    const uint64_t WHOLE      = uint64_t(1) << 32;
    const uint64_t HALF       = uint64_t(1) << 31;
    const uint64_t QUARTER    = uint64_t(1) << 30;
    const uint64_t TQUARTER   = uint64_t(3)*QUARTER;
};

#endif /* ArithmeticCoder_hpp */
