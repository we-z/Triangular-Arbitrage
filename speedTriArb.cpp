#include <iostream>
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <cpprest/uri.h>
#include <cpprest/ws_client.h>
#include <cpprest/containerstream.h>
#include <cpprest/filestream.h>
#include <cpprest/http_listener.h>
#include <cpprest/producerconsumerstream.h>

using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace web::json;

// Alpaca Constants
const std::string API_KEY = config::API_KEY;
const std::string SECRET_KEY = config::SECRET_KEY;

http_request make_request(method mtd, const std::string& url) {
    http_request req(mtd);
    req.headers().add("APCA-API-KEY-ID", API_KEY);
    req.headers().add("APCA-API-SECRET-KEY", SECRET_KEY);
    req.set_request_uri(url);
    return req;
}

web::json::value get_response(const http_response& response) {
    if (response.status_code() != 200) {
        throw std::runtime_error("Undesirable response from Alpaca!");
    }
    return response.extract_json().get();
}

pplx::task<web::json::value> get_quote(const std::string& symbol, http_client& client) {
    const std::string url = "https://data.alpaca.markets/v1beta3/crypto/us/latest/trades?symbols=" + symbol;
    return client.request(make_request(methods::GET, url)).then(get_response);
}

pplx::task<void> check_arb(web::json::value& prices, http_client& client) {
    // Get the prices
    double ETH = prices.at(U("ETH/USD")).as_double();
    double BTC = prices.at(U("BTC/USD")).as_double();
    double ETHBTC = prices.at(U("ETH/BTC")).as_double();
    double DIV = ETH / BTC;
    double BUY_ETH = 1000 / ETH;
    double BUY_BTC = 1000 / BTC;

    // When BTCUSD is cheaper
    if (DIV > ETHBTC * (1 + min_arb_percent / 100)) {
        return post_Alpaca_order("BTCUSD", BUY_BTC, "buy", client).then([=](http_response order1) {
            if (order1.status_code() == 200) {
                return post_Alpaca_order("ETH/BTC", BUY_ETH * 0.95, "buy", client).then([=](http_response order2) {
                    if (order2.status_code() == 200) {
                        return liquidate(client).then([=](http_response order3) {
                            if (order3.status_code() == 207) {
                                std::cout << "Done (type 1) eth: " << ETH << " btc: " << BTC << " ethbtc " << ETHBTC << std::endl;
                            }
                            else {
                                liquidate(client).then([](http_response) {
                                    std::cout << "Bad Order 3 BTC" << std::endl;
                                });
                            }
                        });
                    }
                    else {
                        liquidate(client).then([=](http_response) {
                            std::cout << order2.to_string() << std::endl;
                            std::cout << "Bad Order 2 BTC" << std::endl;
                        });
                    }
                });
            }
            else {
                std::cout << "Bad Order 1 BTC" << std::endl;
                return pplx::task_from_result();
            }
        });
    }
