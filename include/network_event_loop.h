#ifndef __NETWORK_EVENT_LOOP_H_INCLUDED__
#define __NETWORK_EVENT_LOOP_H_INCLUDED__

#include <assert.h>
#include <queue>
#include <algorithm>
#include <unistd.h>

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>

class network_event_loop
{
    public:
        network_event_loop (const char* self, const char* next)
        : pull_socket_(-1)
        , push_socket_(-1)
        , push_attempts_(0)
        , pull_attempts_(0)
        {
            init_inbox  (self);
            init_outbox (next);

            std::cout << "Starting event loop...\n";
            std::cout << "Listening on " << self << std::endl;
        }

        ~network_event_loop ()
        {
            nn_shutdown (push_socket_, 0);
        }

        virtual void recv(std::string&) = 0;
        virtual void rd_timeout() = 0;
        virtual void wr_timeout() = 0;

        void send(std::string str)
        {
            outbox_.push(new std::string(str));
        }

        void run()
        {
            while(true)
            {
                //connect (self, next);

                if (rd_message ())
                     execute ();

                 wr_message();
                //if (wr_message())
                   //  std::cout << "sent message\n";
                usleep(500);
            }
        }


    protected:

        void init_inbox (const char* self)
        {
            pull_socket_ = nn_socket (AF_SP, NN_PULL);
            assert (pull_socket_ >= 0);

            if (nn_bind (pull_socket_, self))
                pull_errno_ = nn_errno ();
        }

        void init_outbox (const char* next)
        {
            //std::cout << "Setting next as " << next << "\n";
            if(push_socket_ >= 0)
            {
                nn_shutdown (push_socket_, 0);
            }

            push_socket_ = nn_socket (AF_SP, NN_PUSH);
            assert(push_socket_ >= 0);

            if(nn_connect (push_socket_, next) < 0)
                push_errno_ = nn_errno ();

            //std::cout << "next = " << next << "\n";
        }

        bool connect (const char* bind, const char* next)
        {
            // if (push_errno_ == ENODEV)
            // {
            //     std::cout << "fixing connect()\n";
            //     nn_connect (push_socket_, next);
            //     push_errno_ = nn_errno ();
            // }

            // if (pull_errno_)
            // {
            //     std::cout << "fixing bind()\n";
            //     nn_bind (pull_socket_, bind);
            //     pull_errno_ = nn_errno ();
            // }

            return ((push_errno_ | push_errno_) == 0);
        }

        bool rd_message ()
        {
            char* buf = NULL;

            int  bytes = nn_recv (pull_socket_, &buf, NN_MSG, NN_DONTWAIT);
            bool valid = (bytes > 0);

            if (valid)
            {
                inbox_.push (new std::string(buf, bytes));
                nn_freemsg(buf);
                pull_attempts_ = 0;
            }

            if(pull_attempts_++ >= MAX_PULL_ATTEMPTS)
            {
                pull_attempts_ = 0;
                rd_timeout();
            }

            return valid;
        }

        bool wr_message ()
        {
            bool valid = false;
            if (!outbox_.empty ())
            {
                std::string*& out = outbox_.front ();
                
                int bytes = nn_send (push_socket_, out->c_str (), out->size (), NN_DONTWAIT);
                    valid = (bytes == static_cast<int> (out->size ()));
                
                if (valid)
                {
                    outbox_.pop();
                    delete out;
                    push_attempts_ = 0;
                }

                if(push_attempts_++ >= MAX_PUSH_ATTEMPTS)
                {
                    push_attempts_ = 0;
                    wr_timeout();
                }
            }


            return valid;
        }


        void execute ()
        {
            std::string*& in = inbox_.front();
                               inbox_.pop();

            recv(*in);
            delete in;
           // std::cout << "Queues: " << rd_size() << " " << wr_size() << "\n";

            //std::cout << "pull: " << pull_attempts_ << "\t push: " << push_attempts_ << "\n";
        }

        const size_t rd_size() const { return inbox_.size(); }
        const size_t wr_size() const { return outbox_.size(); }


        int pull_socket_;
        int push_socket_;

        int push_errno_;
        int pull_errno_;

        std::queue<std::string*> inbox_;
        std::queue<std::string*> outbox_;

        const static int MAX_PUSH_ATTEMPTS = 1E3;
        const static int MAX_PULL_ATTEMPTS = 1E3;
        int push_attempts_;
        int pull_attempts_;
};


#endif