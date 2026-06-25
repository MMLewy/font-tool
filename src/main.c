#include <application.h>
#include <globals.h>

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    if(app_init())
    {
        if(app_run()) return 0;
        else return -2;
    }
    
    return -1;
}
