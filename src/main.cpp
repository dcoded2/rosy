#include <iostream>
#include <map>

#include <network_event_controller.h>


class test_controller : public network_event_controller {
    public:
        test_controller(const char* self, const char* next, const char* pubsub)
        : network_event_controller(self, next, pubsub) {}

        void event(std::string& protobuf)
        {
            std::cout << "test() whoohoo!\n";

            this->send(protobuf);
        }
};

int main(int argc, char** argv)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    std::cout << NN_SUB << std::endl;

    char* self = argv[1];
    char* peer = argv[2];
    char* psch = argv[3];

    std::cout << "Self    : " << self << std::endl;
    std::cout << "Peer    : " << peer << std::endl;
    std::cout << "Channel : " << psch << std::endl;
    std::cout << "-----------------------------------------" << std::endl;


    test_controller nec(self, peer, psch);
    
    nec.run();
}

