#include "ArithmeticCoder.hpp"

obstream::obstream(const std::string &filepath)
{
    fout.open(filepath, std::ofstream::binary);
    buffer = std::string(BUFFER_SIZE, 0);
    
    byte_pos = 0;
    bit_pos  = 7;
}

obstream& obstream::operator<<(bool bit)
{
    //std::cout << bit;
    if (bit) buffer[byte_pos] |= 1 << bit_pos;
    
    next_bit();
    
    return *this;
}

obstream& obstream::operator<<(uint64_t num)
{
    for (uint32_t d = 63; d < 64; d--) this->operator<<(bool((num >> d) & 1));
    
    return *this;
}

void obstream::flush()
{
    fout.write(buffer.data(), byte_pos+1);
    fout.close();
}

void obstream::next_bit()
{
    if (--bit_pos >= 8)
    {
        bit_pos = 7;
        byte_pos++;
        
        if (byte_pos == BUFFER_SIZE)
        {
            fout.write(buffer.data(), BUFFER_SIZE);
            
            byte_pos = 0;
            bit_pos  = 7;
        }
        
        buffer[byte_pos] = 0;
    }
}



ibstream::ibstream(const std::string &filepath, uint64_t offset)
{
    buffer = std::string(BUFFER_SIZE, 0);
    
    fin.open(filepath, std::ofstream::binary);
    fin.read(buffer.data(), offset);
    fin.read(buffer.data(), BUFFER_SIZE);
    
    byte_pos    = 0;
    bit_pos     = 7;
}

bool ibstream::eof()
{
    return fin.gcount() == 0;
}

bool ibstream::read()
{
    bool bit = (buffer[byte_pos] >> bit_pos) & 1;
    
    next_bit();
    
    return bit;
}

void ibstream::next_bit()
{
    if (--bit_pos >= 8)
    {
        bit_pos = 7;
        byte_pos++;
        
        if (byte_pos >= fin.gcount())
        {
            fin.read(buffer.data(), BUFFER_SIZE);
            
            byte_pos = 0;
        }
    }
}




ArithmeticCoder::ArithmeticCoder(Model& input_model)
: model(input_model)
{
    
}

std::string get_encoding_filepath(const std::string& filepath)
{
    std::size_t pos = filepath.find_last_of('.');
    
    if (pos == std::string::npos)   return filepath                 + ".iac";
    else                            return filepath.substr(0, pos)  + ".iac";
}

void ArithmeticCoder::encode(const std::string &filepath)
{
    model.reset();
    
    std::ifstream   fin(filepath, std::ifstream::binary);
    std::string     buffer_in(BUFFER_SIZE, '\0');
    uint64_t        size = std::filesystem::file_size(filepath);
    std::streamsize read = size;
    
    obstream fout(get_encoding_filepath(filepath));
    fout << size;
    
    // Encoding
    uint64_t lower = 0;
    uint64_t upper = WHOLE;
    uint8_t  s     = 0;
    
    TimerMeasure START = Timer::now();
    while (!fin.eof() && read > 0)
    {
        fin.read(buffer_in.data(), BUFFER_SIZE);
        read = fin.gcount();
        
        for (std::size_t i = 0; i < read; i++)
        {
            uint8_t byte = reinterpret_cast<uint8_t&>(buffer_in[i]);
            
            uint64_t w = upper - lower;
            
            uint64_t cdf = model.cdf(byte);
            uint64_t f   = model.frq(byte);
            uint64_t sum = model.sum();
            
            upper = lower + std::llround( double(w) * double(cdf + f) / double(sum) );
            lower = lower + std::llround( double(w) * double(cdf)     / double(sum) );
            
            // Rescaling
            while (true)
            {
                if (upper < HALF)
                {
                    // emit [0] + s*[1]
                    fout << false;
                    for (uint8_t k = 0; k < s; k++) fout << true;
                    
                    s = 0;
                }
                else if (lower > HALF)
                {
                    // emit [1] + s*[0]
                    fout << true;
                    for (uint8_t k = 0; k < s; k++) fout << false;
                    
                    s = 0;
                    lower -= HALF;
                    upper -= HALF;
                }
                else if (lower > QUARTER && upper < TQUARTER)
                {
                    s++;
                    lower -= QUARTER;
                    upper -= QUARTER;
                }
                else break;
                
                lower <<= 1;
                upper <<= 1;
            }
            
            // update model
            model.update(byte);
        }
    }
    
    // Flush
    s++;
    if (lower <= QUARTER)
    {
        // emit [0] + s*[1]
        fout << false;
        for (uint8_t k = 0; k < s; k++) fout << true;
    }
    else
    {
        // emit [1] + s*[0]
        fout << true;
        for (uint8_t k = 0; k < s; k++) fout << false;
    }
    
    fout.flush();
    
    TimerMeasure END = Timer::now();
    std::cout << "time: " << std::chrono::duration<double>(END - START).count() << std::endl;
}

void ArithmeticCoder::decode(const std::string &filepath)
{
    model.reset();
    
    std::ofstream   fout(filepath.substr(0, filepath.size() - 4) + "_iac.txt", std::ofstream::binary);
    std::string     buffer_out(BUFFER_SIZE, '\0');
    std::size_t     buffer_pos = 0;
    uint64_t        output_size = 0;
    
    ibstream        fin(filepath, 8);
    
    // Get metadata
    std::ifstream   fmeta(filepath, std::ifstream::binary);
    std::string     buffer_meta(8, 0);
    uint64_t        length = 0;
    fmeta.read(buffer_meta.data(), 8);
    fmeta.close();
    
    for (std::size_t i = 0; i < 8; i++) for (std::size_t j = 7; j < 8; j--)
    {
        bool    bit     = ((buffer_meta[i] >> j) & 1);
        uint8_t offset  = 63 - 8*i - (7 - j);
        
        length |= (uint64_t(bit) << offset);
    }
    
    // Setup decoding
    uint64_t lower  = 0;
    uint64_t upper  = WHOLE;
    uint64_t z      = 0;
    std::size_t i   = 1;
    while (i <= 32 && !fin.eof())
    {
        z += fin.read() * (uint64_t(1) << (32 - i));
        i++;
    }
    
    // Decoding
    TimerMeasure START = Timer::now();
    while (true)
    {
        uint64_t cdf = 0;
        uint64_t f;
        uint8_t  character;
        model.init_search();
        while (model.next(character, f))
        {
            uint64_t w   = upper - lower;
            uint64_t sum = model.sum();
            
            uint64_t upper_new = lower + std::llround( double(w) * double(cdf + f) / double(sum) );
            uint64_t lower_new = lower + std::llround( double(w) * double(cdf)     / double(sum) );
            
            if (lower_new <= z && z < upper_new)
            {
                buffer_out[buffer_pos] = reinterpret_cast<char&>(character);
                buffer_pos++;
                output_size++;
                if (buffer_pos >= BUFFER_SIZE)
                {
                    fout.write(buffer_out.data(), BUFFER_SIZE);
                    buffer_pos = 0;
                }
                
                if (output_size >= length)
                {
                    // flush
                    fout.write(buffer_out.data(), buffer_pos);
                    fout.close();
                    
                    TimerMeasure END = Timer::now();
                    std::cout << "time: " << std::chrono::duration<double>(END - START).count() << std::endl;
                    return;
                }
                
                lower = lower_new;
                upper = upper_new;
                
                // update model
                model.update(character);
                
                break;
            }
            
            cdf += f;
        }
            
        while (upper < HALF || lower > HALF)
        {
            if (upper < HALF)
            {
                lower   = lower << 1;
                upper   = upper << 1;
                z       = z << 1;
            }
            else if (lower > HALF)
            {
                lower   = (lower - HALF) << 1;
                upper   = (upper - HALF) << 1;
                z       = (z     - HALF) << 1;
            }
            
            if (!fin.eof() && fin.read()) z++;
            i++;
        }
        
        while (upper < TQUARTER && lower > QUARTER)
        {
            lower   = (lower - QUARTER) << 1;
            upper   = (upper - QUARTER) << 1;
            z       = (z     - QUARTER) << 1;
            
            if (!fin.eof() && fin.read()) z++;
            i++;
        }
    }
}
