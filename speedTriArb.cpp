#include <iostream>
#include <string>
#include <curl/curl.h>
#include "config.hpp"

using namespace std;

// Alpaca Constants
const string API_KEY = Config::API_KEY;
const string SECRET_KEY = Config::SECRET_KEY;
const string ALPACA_BASE_URL = "https://paper-api.alpaca.markets";
const string DATA_URL = "https://data.alpaca.markets";
const double min_arb_percent = 0.1;

// Callback function for CURL
size_t write_callback(char* ptr, size_t size, size_t nmemb, string* data)
{
    size_t realsize = size * nmemb;
    data->append(ptr, realsize);
    return realsize;
}

// Function to get quote data from Alpaca API
double get_quote(string symbol)
{
    try
    {
        // initialize CURL
        CURL* curl = curl_easy_init();

        // set CURL options
        string url = DATA_URL + "/v1beta3/crypto/us/latest/trades?symbols=" + symbol;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_slist_append(NULL, ("APCA-API-KEY-ID: " + API_KEY).c_str()));
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_slist_append(NULL, ("APCA-API-SECRET-KEY: " + SECRET_KEY).c_str()));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

        // make the request
        string response_string;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        CURLcode res = curl_easy_perform(curl);

        // check for errors
        if (res != CURLE_OK)
        {
            cout << "There was an issue getting trade quote from Alpaca: " << curl_easy_strerror(res) << endl;
            return false;
        }

        // parse the JSON response
        string start_key = "\"p\":";
        size_t start = response_string.find(start_key) + start_key.length();
        size_t end = response_string.find(",", start);
        string price_string = response_string.substr(start, end - start);
        return stod(price_string);
    }
    catch (exception const& e)
    {
        cout << "There was an issue getting trade quote from Alpaca: " << e.what() << endl;
        return false;
    }
}

// Function to post an order to Alpaca
bool post_Alpaca_order(string symbol, int qty, string side)
{
    try
    {
        // initialize CURL
        CURL* curl = curl_easy_init();

        // set CURL options
        curl_easy_setopt(curl, CURLOPT_URL, (ALPACA_BASE_URL + "/v2/orders").c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_slist_append(NULL, ("APCA-API-KEY-ID: " + API_KEY).c_str()));
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_slist_append(NULL, ("APCA-API-SECRET-KEY: " + SECRET_KEY).c_str()));
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, ("symbol=" + symbol + "&qty=" + to_string(qty) + "&side=" + side + "&type=market&time_in_force=gtc").c_str());

        // make the request
        CURLcode res = curl_easy_perform(curl);

        // check for errors
        if (res != CURLE_OK)
        {
            cout << "There was an issue posting order to Alpaca
