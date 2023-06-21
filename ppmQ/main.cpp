#include "ArithmeticCoder.hpp"


int main(int argc, const char * argv[])
{
    std::string filepath_toencode = "/data/gpfs/projects/COMP90055/txtcomp/metawiki-latest-iwlinks-small";
    std::string filepath_todecode = "/data/gpfs/projects/COMP90055/txtcomp/metawiki-latest-iwlinks.iac";
    
    std::vector<uint8_t> orders = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    //std::vector<uint32_t> max_contexts = {1000, 10000, 100000, 1000000, 10000000, std::numeric_limits<uint32_t>::max()};
    std::vector<uint32_t> max_contexts = {1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000, std::numeric_limits<uint32_t>::max()};
    std::vector<uint32_t> mults = {2, 3, 5, 8};
    
    
    for (uint8_t order : orders) for (uint32_t max_context : max_contexts) for (uint32_t mult : mults)
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
