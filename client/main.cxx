#include <iostream>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "udp_client_v2.h"

using boost::asio::ip::udp;

int main(int argc, char** argv)
{
    boost::program_options::options_description description("Ozzy UDP server options");
    description.add_options()
    (
        "help", "Display this message"
    )
    (
        "x",
        boost::program_options::value<double>(),
        "Up bound for the doubles set"
    );

    boost::program_options::variables_map variables_map;
    try
    {
        boost::program_options::store (boost::program_options::parse_command_line(argc, argv, description), variables_map);
        boost::program_options::notify(variables_map);
    }
    catch(const boost::program_options::error& e)
    {
        std::cerr << "Error parsing command-line arguments: " << e.what() << std::endl;
        exit(-1);
    }


    if (variables_map.contains("help"))
    {
        std::cerr << description << std::endl;
        return 0;
    }

    double x = 1.0;
    if (variables_map.contains("x"))
    {
        x = variables_map["x"].as<double>();
        std::cerr << "Set X to " << x << std::endl;
    }

    try
    {
        boost::asio::io_context io_context;
        std::vector<std::thread> client_threads;

        const int num_clients = 4;

        for (int i = 0; i < num_clients; ++i)
        {
            client_threads.emplace_back([&io_context, x, i]
            {
                try
                {
                    Ozzy::v2::UdpClient client(io_context, "config/client_cfg.cfg", "UdpClient" + std::to_string(i), x);
                    client.process_handshake();
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Exception in client thread: " << e.what() << std::endl;
                }
            });
        }
        io_context.run();

        for (auto &thread: client_threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
