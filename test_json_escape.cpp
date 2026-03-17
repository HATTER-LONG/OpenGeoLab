#include <iostream>
#include <nlohmann/json.hpp>

int main() {
    nlohmann::json j = {{"key", "value with ''' in it"}};
    std::cout << j.dump(2) << std::endl;
    
    // Also test what the Python code would look like
    std::ostringstream script;
    script << "request = json.loads(r'''" << j.dump(2) << "''')\n";
    std::cout << "\nGenerated Python snippet:\n" << script.str() << std::endl;
    return 0;
}
