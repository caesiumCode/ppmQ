#include "ArithmeticCoder.hpp"


int main(int argc, const char * argv[])
{
    //AdaptativeDictionaryModel model;
    PPMModel model;
    model.set_order(5);
    
    ArithmeticCoder coder(model);
    
    std::string filepath_toencode = "/Users/stephen/Desktop/Research Assistant/text compression/dataset/enwik8";
    std::string filepath_todecode = "/Users/stephen/Desktop/Research Assistant/text compression/dataset/enwik8.iac";
    
    coder.encode(filepath_toencode);
    coder.decode(filepath_todecode);
    
    return 0;
}
