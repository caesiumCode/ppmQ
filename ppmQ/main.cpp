#include "ArithmeticCoder.hpp"
#include "AdaptativeDictionaryModel.hpp"
#include "DictionaryModel.hpp"



int main(int argc, const char * argv[])
{
    //DictionaryModel model;
    AdaptativeDictionaryModel model;
    
    ArithmeticCoder coder(model);
    
    std::string filepath_toencode = "/Users/stephen/Desktop/Research Assistant/text compression/dataset/enwik9";
    std::string filepath_todecode = "/Users/stephen/Desktop/Research Assistant/text compression/dataset/enwik9.iac";
    
    coder.encode(filepath_toencode);
    //model.disp();
    coder.decode(filepath_todecode);
    //model.disp();
    
    return 0;
}
