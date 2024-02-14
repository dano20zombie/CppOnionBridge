#include "connection.hpp"

// *** DISCLAIMER ***
// Make sure the TOR service is openand configured for port 9150.
// The TOR service can also be installed as a boot service so it runs automatically when the computer starts, or it can be started through code.

static std::string const host = "bible4u2lvhacg4b3to2e2veqpwmrc2c3tjf2wuuqiz332vlwmr4xbad.onion"; // Bible4u
static std::string const api = "/en/about"; // About Page

int main()
{
    std::string response;
    connection.make_get(host, api, std::string(), response);

    std::cout << "Connecting to " << host + api << "..." << std::endl;
    std::cout << response << std::endl;
    system("pause");
}