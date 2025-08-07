#define BOOST_JSON_STANDALONE
#include <boost/json/src.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <string>

#include "logger.hpp"

namespace beast= boost::beast;
namespace websocket= beast::websocket;
namespace net= boost::asio;
namespace json= boost::json;
using tcp= net::ip::tcp;

void do_session(tcp::socket sock)
{
    try {
        websocket::stream<tcp::socket> ws(std::move(sock));
        ws.accept();

        for (;;) {
            beast::flat_buffer buf;
            ws.read(buf);

            std::string raw = beast::buffers_to_string(buf.data());

            logging::LogTransaction log(raw);

            json::object rep;
            rep["processed"]= "HTTP_OKAY";
            rep["input_received"]= json::parse(raw);
            std::string out_raw= json::serialize(rep);

            ws.text(true);
            ws.write(net::buffer(out_raw));

            log.complete(out_raw);
        }
    }
    catch (const beast::system_error& se) {
        if (se.code() != websocket::error::closed)
            std::cerr << "session error: " << se.code().message() << '\n';
    }
    catch (const std::exception& e) {
        std::cerr << "session exception: " << e.what() << '\n';
    }
}

int main()
{
    try {
        net::io_context ioc;
        tcp::acceptor acc(ioc, { tcp::v4(), 8080 });

        std::cout << "[INFO] Server listening on :8080\n";

        for (;;) {
            tcp::socket sock(ioc);
            acc.accept(sock);
            do_session(std::move(sock));
        }
    }
    catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << '\n';
        return 1;
    }
}

