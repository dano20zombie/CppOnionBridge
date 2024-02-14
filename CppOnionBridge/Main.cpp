#include "connection.hpp"

// *** DISCLAIMER ***
// Make sure the TOR service is openand configured for port 9150.
// The TOR service can also be installed as a boot service so it runs automatically when the computer starts, or it can be started through code.

static std::string const host = "bible4u2lvhacg4b3to2e2veqpwmrc2c3tjf2wuuqiz332vlwmr4xbad.onion"; // Bible4u
static std::string const api1 = "/en/about"; // About Page
static std::string const api2 = "/en/links"; // Links Page

int main()
{
    std::string response;
    
    std::cout << "Connecting to " << host + api1 << "..." << std::endl;
    connection.make_get(host, api1, std::string(), response);
    std::cout << response << std::endl;
    std::cout << "Connecting to " << host + api2 << "..." << std::endl;
    connection.make_get(host, api2, std::string(), response);
    std::cout << response << std::endl;
    system("pause");
}