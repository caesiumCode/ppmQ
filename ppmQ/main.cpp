#include "ArithmeticCoder.hpp"



int main(int argc, const char * argv[])
{
    DictionaryModel model;
    
    ArithmeticCoder coder(model);
    
    std::string filepath_toencode = "/Users/stephen/Desktop/Research Assistant/text compression/dataset/enwik8";
    std::string filepath_todecode = "/Users/stephen/Desktop/Research Assistant/text compression/dataset/enwik8.iac";
    
    coder.encode(filepath_toencode);
    coder.decode(filepath_todecode);
    
    return 0;
}
