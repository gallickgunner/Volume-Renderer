#include <iostream>
#include <exception>
#include "RendererGUI.h"

using namespace std;

int main()
{
    RendererGUI vr(1280, 720, "Volume-Renderer");
    try
    {
        vr.run();
    }
    catch(exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
