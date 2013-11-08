#include <iostream>
#include <map>

#include <network_event_controller.h>

int main(int argc, char** argv)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    network_event_controller nec(argv[1], argv[2]);
    
    if(argc > 3)
        nec.heartbeat_start();

    nec.run();
}

