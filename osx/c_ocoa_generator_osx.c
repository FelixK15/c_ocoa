#include "../c_ocoa_generator.h"

int main(int argc, const char** argv)
{
    c_ocoa_code_generator_parameter parameters = c_ocoa_default_code_generator_parameter();
    evaluate_code_generator_argv_arguments(argc, argv, &parameters);

    c_ocoa_code_gen_context context;
    if( !c_ocoa_create_code_gen_context( &context ) )
    {
        return 2;
    }
    
    const int returnValue = c_ocoa_create_classes_api( &parameters, &context );
    if( returnValue != 0 )
    {
        return returnValue;
    }
    
    return 0;
}
