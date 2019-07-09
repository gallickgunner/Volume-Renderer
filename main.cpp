#include <iostream>
#include <exception>
#include "VolumeRenderer.h"

using namespace std;

int main()
{
    VolumeRenderer vr;
    try
    {
        vr.setup();
        vr.start();
    }
    catch(exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
