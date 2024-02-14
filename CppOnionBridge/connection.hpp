#pragma once

//BOOST BEFORE EVERYTHING
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#pragma comment(lib, "libcrypto64MD.lib")
#pragma comment(lib, "libssl64MD.lib")

#include <Windows.h>
#include <iostream>

#define WIN32_LEAN_AND_MEAN
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define COBJMACROS

namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = boost::asio::ssl;
namespace asio = boost::asio;
namespace error = boost::asio::error;
using tcp = asio::ip::tcp;

typedef std::vector<std::string> StringVec;

// *** DISCLAIMER ***
// Make sure the TOR service is openand configured for port 9150.
// The TOR service can also be installed as a boot service so it runs automatically when the computer starts, or it can be started through code.

static asio::io_context io_context;
static tcp::socket global_socket(io_context);
static ssl::stream<tcp::socket&>* ssl_stream = nullptr; // Pointer to SSL stream for reuse

class connection_
{
public:
    http::status make_get(std::string host, std::string api_url, std::string request_params, std::string& response)
    {
        try {
            if (!ssl_stream || last_host.empty() || last_host != host)
            {
                if (ssl_stream) {
                    // Clean up the old SSL stream if switching hosts or if it's no longer valid
                    boost::system::error_code ec;
                    ssl_stream->shutdown(ec);
                    delete ssl_stream;
                    ssl_stream = nullptr;
                    global_socket.close(); // Close the socket if switching hosts
                }

                if (!open_socket_SSL(host)) return http::status::bad_gateway;

                // Re-create the SSL stream for the new connection
                ssl::context ctx(ssl::context::sslv23);
                ssl_stream = new ssl::stream<tcp::socket&>(global_socket, ctx);

                // Perform the SSL/TLS handshake
                ssl_stream->handshake(ssl::stream_base::client);

                last_host = host;
            }

            // Prepare the HTTP request
            std::string target = api_url + request_params;
            http::request<http::string_body> req(http::verb::get, target, 11);
            req.set(http::field::host, host);
            req.set(http::field::user_agent, "Mozilla/5.0 (iPhone; CPU iPhone OS 17_0_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.2 Mobile/15E148 Safari/604.1");
            req.set(http::field::connection, "keep-alive");

            // Send the HTTP request over the SSL connection
            http::write(*ssl_stream, req);

            // Receive the HTTP response
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;
            http::read(*ssl_stream, buffer, res);

            // Convert the response body into a string
            response = beast::buffers_to_string(res.body().data());

            // Note: We don't shutdown and delete the SSL stream here to allow for reuse

            return static_cast<http::status>(res.result_int());
        }
        catch (boost::system::system_error const& se) {
            // Handle specific errors and perform necessary cleanup
            close_socket_SSL(); // Ensure the socket is closed on error
            if (ssl_stream) {
                delete ssl_stream;
                ssl_stream = nullptr;
            }

            // Map specific errors to HTTP status codes
            switch (se.code().value())
            {
            case boost::asio::error::timed_out:
                return http::status::request_timeout;
            case boost::asio::error::host_not_found:
            case boost::asio::error::host_unreachable:
                return http::status::service_unavailable;
            default:
                return http::status::unknown;
            }
        }
    }

    http::status make_post(std::string host, std::string api_url, std::string request_params, std::string data, std::string& response)
    {
        try {
            static ssl::stream<tcp::socket&>* ssl_stream = nullptr; // Pointer to SSL stream for reuse
            if (!ssl_stream || last_host.empty() || last_host != host)
            {
                if (ssl_stream) {
                    // Clean up the old SSL stream if switching hosts or if it's no longer valid
                    boost::system::error_code ec;
                    ssl_stream->shutdown(ec);
                    delete ssl_stream;
                    ssl_stream = nullptr;
                    global_socket.close(); // Close the socket if switching hosts
                }

                if (!open_socket_SSL(host)) return http::status::bad_gateway;

                // Re-create the SSL stream for the new connection
                ssl::context ctx(ssl::context::sslv23);
                ssl_stream = new ssl::stream<tcp::socket&>(global_socket, ctx);

                // Perform the SSL/TLS handshake
                ssl_stream->handshake(ssl::stream_base::client);

                last_host = host;
            }

            // Prepare the HTTP request
            std::string target = api_url + request_params;
            http::request<http::string_body> req(http::verb::post, target, 11);
            req.set(http::field::host, host);
            req.set(http::field::user_agent, "Mozilla/5.0 (iPhone; CPU iPhone OS 17_0_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.2 Mobile/15E148 Safari/604.1");
            req.set(http::field::connection, "keep-alive");
            req.body() = data;
            req.prepare_payload(); // Ensure the Content-Length header is correctly set

            // Send the HTTP request over the SSL connection
            http::write(*ssl_stream, req);

            // Receive the HTTP response
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;
            http::read(*ssl_stream, buffer, res);

            // Convert the response body into a string
            response = beast::buffers_to_string(res.body().data());

            // Note: We don't shutdown and delete the SSL stream here to allow for reuse

            return static_cast<http::status>(res.result_int());
        }
        catch (boost::system::system_error const& se) {
            // Handle specific errors and perform necessary cleanup
            close_socket_SSL(); // Ensure the socket is closed on error
            if (ssl_stream) {
                delete ssl_stream;
                ssl_stream = nullptr;
            }

            // Map specific errors to HTTP status codes
            switch (se.code().value())
            {
            case boost::asio::error::timed_out:
                return http::status::request_timeout;
            case boost::asio::error::host_not_found:
            case boost::asio::error::host_unreachable:
                return http::status::service_unavailable;
            default:
                return http::status::unknown;
            }
        }
    }

private:
    std::string last_host;

    bool open_socket_SSL(std::string host)
    {
        try {
            // Return if the socket connection is already open.
            if (global_socket.is_open() && host == last_host)
                return true;

            // *** DISCLAIMER ***
            // Make sure the TOR service is openand configured for port 9150.
            // The TOR service can also be installed as a boot service so it runs automatically when the computer starts, or it can be started through code.

            // TOR configuration on port 9150.
            tcp::resolver resolver(io_context);
            auto endpoint = resolver.resolve("127.0.0.1", "9150");

            global_socket = tcp::socket(io_context);
            boost::asio::connect(global_socket, endpoint);

            // Configure the socket to use SOCKS5 and navigate the TOR network.
            // The SOCKS5 specifications are available at https://tools.ietf.org/html/rfc1928.
            // .onion requests will be similar to any other SOCKS5 request, but the address will be your .onion address and the port will be the one specified by the TOR service.

            // Send the SOCKS5 greeting.
            global_socket.send(boost::asio::buffer("\x05\x01\x00", 3));

            // Receive the response to the SOCKS5 greeting.
            char response[2];
            size_t length = global_socket.receive(boost::asio::buffer(response, 2));

            // Check the response to the SOCKS5 greeting.
            if (length != 2 || response[0] != '\x05' || response[1] != '\x00')
                return false;

            // Send the SOCKS5 connection request.
            //      0x5 : Version \
                    0x1 : CONNECT command \
                    0x0 (third byte) : reserved (Must be NULL) \
                    0x3 : address type (DOMAINNAME)

            char* request = new char[256];
            memset(request, 0, sizeof(request));

            sprintf(request, "\x05\x01\xCC\x03%c%s\x01\xBB", static_cast<char>(host.size()), host.c_str());
            request[2] = '\x00';

            // \x01\xBB : Add the port (in network byte order) (443 in case of HTTPS)

            // Send the request.
            global_socket.send(boost::asio::buffer(request, 7 + host.size()));

            delete[] request;

            // Receive the response to the SOCKS5 connection request.
            char buffer[10];
            int lengthasd = global_socket.receive(boost::asio::buffer(buffer, 10));

            if (lengthasd != 10 || buffer[0] != '\x05' || buffer[1] != '\x00') {
                switch (buffer[1])
                {
                case '\x01':
                    throw http::status::bad_gateway; // General failure of the socket
                    return false;
                case '\x02':
                    throw http::status::method_not_allowed; // Connection's rule not allowed
                    return false;
                case '\x03':
                    throw http::status::not_found; // Network not available
                    return false;
                case '\x04':
                    throw http::status::not_found; // Host not available
                    return false;
                case '\x05':
                    throw http::status::service_unavailable; // Connection refused (SOCK5 not available)
                    return false;
                case '\x06':
                    throw http::status::request_timeout; // Timeout
                    return false;
                case '\x00':
                case '\x07':
                case '\x08':
                default:
                    throw http::status::unknown;
                    return false;
                }
            }

            return true;
        }
        catch (boost::system::system_error const& se) {
            //TOR is not running
            return false;
        }
    }

    bool close_socket_SSL()
    {
        try {
            // Check if the socket is open, if so, close it.
            if (global_socket.is_open()) {
                // The socket is open
                global_socket.close();
            }

            return true;
        }
        catch (boost::system::system_error const& se) {
            return true;
        }
    }

};

static connection_ connection;