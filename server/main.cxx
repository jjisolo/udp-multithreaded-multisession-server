#include <iostream>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include "udp_server_v2.h"

auto main(int argc, char** argv) -> int
{
    //
    // Setup program options
    //
    boost::program_options::options_description description("Ozzy UDP server options");
    description.add_options()
    (
        "help", "Display this message"
    )
    (
        "doubles",
        boost::program_options::value<std::uint64_t>(),
        "Count of the doubles to send to the client"
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

    std::uint64_t doubles_count = 1000000;
    if (variables_map.contains("doubles"))
    {
        doubles_count = variables_map["doubles"].as<std::uint64_t>();
        std::cerr << "Set count of sended doubles to the " << doubles_count << std::endl;
    }

    //
    // Initialize main routine
    //
    try
    {
        boost::asio::io_context context;
        Ozzy::v2::UdpServer     server(context, "config/server_cfg.cfg", "UdpServer", doubles_count);
        server.start();

        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
