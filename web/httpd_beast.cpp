
#include <boost/beast.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <giolib/static_block.h>
#include <giolib/main.h>

#include "httpd_beast.h"

#include "utils/utils.h"

void handle_connection(boost::asio::ip::tcp::socket socket) {
    boost::system::error_code ec;
    boost::beast::flat_buffer buffer;
    bool close = false;

    while (!close) {
        boost::beast::http::request< boost::beast::http::string_body > req;
        boost::beast::http::read(socket, buffer, req, ec);
        if (ec == boost::beast::http::error::end_of_stream) {
            break;
        }
        assert(!ec);
        boost::beast::http::string_body::value_type body = "Test";
        auto size = body.size();
        boost::beast::http::response< boost::beast::http::string_body > res(std::piecewise_construct, std::make_tuple(std::move(body)), std::make_tuple(boost::beast::http::status::ok, req.version()));
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/plain");
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        close = res.need_eof();
        boost::beast::http::write(socket, res, ec);
        assert(!ec);
    }

    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
}

int beast_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    const auto address = boost::asio::ip::make_address_v6("::ffff:127.0.0.1");
    const short port = 8080;

    boost::asio::io_context ioc;
    boost::asio::ip::tcp::acceptor acceptor(ioc, {address, port});
    std::list< std::thread > children_threads;
    while (true) {
        boost::asio::ip::tcp::socket socket(ioc);
        acceptor.accept(socket);
        children_threads.emplace_back([socket=std::move(socket)]() mutable { handle_connection(std::move(socket)); });
    }

    return 0;
}
gio_static_block {
    gio::register_main_function("beast", beast_main);
}
