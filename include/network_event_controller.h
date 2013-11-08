#ifndef __INCLUDE_NETWORK_EVENT_CONTROLLER_H__
#define __INCLUDE_NETWORK_EVENT_CONTROLLER_H__

#include <set>
#include <sys/time.h>

#include <network_event_loop.h>
#include <messages.pb.h>

class network_event_controller : public network_event_loop {
    protected:
        // handler registration
        typedef void (network_event_controller::*event_handler)(std::string&);
        std::map<std::string, event_handler> handlers_;

        // heartbeat and routing
        std::string self_;

        //std::set<std::string> peers_;
        std::map<std::string, std::string> peers_;
        std::vector<std::string> dropped_;

        bool joined_;
        int  last_heartbeat_;

    public:

        virtual void event(std::string&)
        {
            
        }

        network_event_controller (const char* self, const char* next, const char* pubsub)
        : network_event_loop(self, next, pubsub)
        , self_(self)
        , joined_(false)
        , last_heartbeat_(0)
        {
            handlers_["heartbeat"] = &network_event_controller::heartbeat_event;
            handlers_["join"]      = &network_event_controller::join_event;
            handlers_["drop"]      = &network_event_controller::drop_event;

            peers_[self] = next;

            heartbeat_start();
            join_init();
        }

        ~network_event_controller() {}

        void recv(std::string& protobuf)
        {
            beta::base base;
            base.ParseFromString(protobuf);

        //    std::cout << base.type() << "\n";

            std::map<std::string, event_handler>::iterator it = handlers_.find(base.type());

            if(it != handlers_.end())
            {
                (this->*(it->second))(protobuf);
            }   
            else
            {
                event(protobuf);
                std::cout << "unknown packet type: " << base.type() << "\n";
            }       
        }

        void wr_timeout()
        {
            if(joined_)
                update_next_peer();
        }

        void rd_timeout()
        {
            heartbeat_start();
        }


        void update_next_peer()
        {
            drop_peer_init(peers_[self_]);
            std::cout << "init_drop_peer(" << peers_[self_] << ")\n";
            const std::string next = peers_[self_];

            std::cout << "next:  " << peers_[self_] << " -> " << peers_[next] << "\n";


            std::string new_next = peers_[next];
            peers_[self_] = new_next;
            peers_.erase(next);

            init_outbox(new_next.c_str());
        }





        void join_event(std::string& protobuf)
        {
            beta::join join;
            join.ParseFromString(protobuf);

            join_add_peer(join);
        }

        void join_init()
        {
            beta::join join;
            join.set_type("join");

            beta::endpoint* self = join.mutable_self();
            self->set_addr(self_);
            self->set_next(peers_[self_]);

            this->send(join.SerializeAsString());
            std::cout << "sent join request\n";
        }

        void join_add_peer(beta::join& join)
        {
            std::string peer = join.self().addr();
            std::string next = join.self().next();

            if(peer == self_)
            {
                joined_ = true; // if we got our own join req. we're in.
                return;
            }

            if(next == peers_[self_])
            {
                peers_[self_] = peer;
                init_outbox(peer.c_str());
            }

            std::cout << "added peer " << peer << std::endl;
            peers_[peer] = next;

            this->send(join.SerializeAsString()); // otherwise pass it on
        }



        void drop_event(std::string& protobuf)
        {
            beta::drop drop;
            drop.ParseFromString(protobuf);

            const beta::endpoint& endpoint = drop.self();

            std::cout << "drop(" << endpoint.addr() << ")\n";

            if(peers_.find(endpoint.addr()) != peers_.end())
            {
                peers_.erase(endpoint.addr());
                dropped_.push_back(endpoint.addr());
                this->send(protobuf);
            }
        }

        void drop_peer_init(const std::string& bad_peer)
        {
            beta::drop drop;
            drop.set_type("drop");

            beta::endpoint* self = drop.mutable_self();
            self->set_addr(bad_peer);
            self->set_next(bad_peer);

            this->send(drop.SerializeAsString());
        }

        


        void heartbeat_event(std::string& protobuf)
        {
            beta::heartbeat heartbeat;
            heartbeat.ParseFromString(protobuf);

            if(heartbeat.created() >= last_heartbeat_)
            {
                last_heartbeat_ = heartbeat.created();

                heartbeat_routing_update(heartbeat);
                //heartbeat_print(heartbeat);

                heartbeat.set_cycles(heartbeat.cycles() + 1);

                this->send(heartbeat.SerializeAsString());
            }
            else
            {
                std::cout << "old heartbeat\n";
            }
        }

        void heartbeat_start()
        {
            beta::heartbeat heartbeat;
            timespec now;

            clock_gettime(CLOCK_MONOTONIC, &now);

            heartbeat.set_type("heartbeat");
            heartbeat.set_cycles(heartbeat.cycles() + 1);
            heartbeat.set_created(now.tv_sec);

            heartbeat_routing_update(heartbeat);
            
            this->send(heartbeat.SerializeAsString());
        }

        void heartbeat_print(beta::heartbeat& heartbeat)
        {
            std::cout << "next     : " << peers_[self_] << "\n";

            std::cout << "Protobuf : " << heartbeat.GetTypeName() << "\n";
            std::cout << "cycles   : " << heartbeat.cycles() << std::endl;
            std::cout << "created  : " << heartbeat.created() << std::endl;
            std::cout << "endpoints: " << heartbeat.endpoints_size() << "\n";
            for(int i = 0; i < heartbeat.endpoints_size(); i++)
            {
                std::cout << "\t" << heartbeat.endpoints(i).addr() << " -> " << heartbeat.endpoints(i).next() << "\n";
            }
        }


        void heartbeat_routing_update(beta::heartbeat& heartbeat)
        {
            {
                const beta::endpoint* endpoint = NULL;
                for(int i = 0; i < heartbeat.endpoints_size(); i++)
                {
                    endpoint = &(heartbeat.endpoints(i));
                    if(endpoint->addr() != self_)
                    {
                        peers_[endpoint->addr()] = endpoint->next();
                    }
                }
            }

            // remove dropped nodes
            std::vector<std::string>::iterator dropped_it;
            for(dropped_it = dropped_.begin(); dropped_it != dropped_.end(); dropped_it++)
                peers_.erase(*dropped_it);
            dropped_.clear();


            beta::endpoint* endpoint = NULL;
            heartbeat.clear_endpoints();

            std::map<std::string, std::string>::iterator it;
            for(it = peers_.begin(); it != peers_.end(); it++)
            {
                endpoint = heartbeat.add_endpoints();
                endpoint->set_addr(it->first);
                endpoint->set_next(it->second);
            }

        
        }
};


#endif