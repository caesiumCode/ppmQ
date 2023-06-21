#include "ArithmeticCoder.hpp"


int main(int argc, const char * argv[])
{
    std::string filepath_toencode = std::string(argv[1]);
    uint8_t order = atoi(argv[2]);
    
    std::vector<uint32_t> max_contexts = {1000, 10000, 100000, 1000000, 10000000, 100000000};
    std::vector<uint32_t> mults = {1, 2, 3, 5, 8};
    
    PPMModel model;
    model.set_order(order);
    model.set_max_context(std::numeric_limits<uint32_t>::max());
    
    ArithmeticCoder coder(model);
    
    std::string filepath_todecode = coder.get_encoding_filepath(filepath_toencode);
    
    coder.encode(filepath_toencode);
    std::cout << model.get_parameters() << "," << std::filesystem::file_size(filepath_todecode) << std::endl;
    
    for (uint32_t max_context : max_contexts) for (uint32_t mult : mults)
    {
        PPMModel model;
        model.set_order(order);
        model.set_max_context(mult * max_context);
        
        ArithmeticCoder coder(model);
        
        coder.encode(filepath_toencode);
        std::cout << model.get_parameters() << "," << std::filesystem::file_size(filepath_todecode) << std::endl;
    }
    
    return 0;
}
