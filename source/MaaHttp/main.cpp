#include <iostream>

#include "info.hpp"

int main()
{
    using in = lhg::convert_input<lhg::maa::func_type_MaaResourceGetTaskList::args>;
    using out = lhg::convert_output<lhg::maa::func_type_MaaResourceGetTaskList::args>;
    std::cout << typeid(in).name() << std::endl;
    std::cout << typeid(out).name() << std::endl;
}