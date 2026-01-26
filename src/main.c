#include <application.h>

int main(int argc, char *argv[])
{
    if(app_init())
    {
        if(app_run()) return 0;
        else return -2;
    }
    
    return -1;
}
