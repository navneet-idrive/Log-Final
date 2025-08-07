#pragma once
#include <boost/json.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <curl/curl.h>
#include <chrono>
#include <string>

namespace logging {

inline long long epoch_ms()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(
               system_clock::now().time_since_epoch()).count();
}

inline std::string b64(std::string const& s)
{
    using namespace boost::archive::iterators;
    using It = base64_from_binary<
                 transform_width<std::string::const_iterator, 6, 8>>;
    std::string out(It(s.begin()), It(s.end()));
    out.append((3 - s.size() % 3) % 3, '=');
    return out;
}

inline void curl_send(boost::json::object const& j)
{
    std::string form = "data=" + b64(boost::json::serialize(j));

    CURL* curl = curl_easy_init();
    if (!curl) return;
    curl_easy_setopt(curl,CURLOPT_URL,"http://127.0.0.1:3300/log");
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,form.c_str());
    curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,form.size());
    struct curl_slist* hdr = nullptr;
    hdr = curl_slist_append(hdr,"Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl,CURLOPT_HTTPHEADER,hdr);
    curl_easy_setopt(curl,CURLOPT_IPRESOLVE,CURL_IPRESOLVE_V4);
    curl_easy_perform(curl);
    curl_slist_free_all(hdr);
    curl_easy_cleanup(curl);
}

class LogTransaction{
    std::string input_;
    long long in_ts_;
    bool done_ = false;

public:
    explicit LogTransaction(std::string input_raw)
        : input_(std::move(input_raw))
        , in_ts_(epoch_ms())
    {}

    void complete(std::string const& output_raw)
    {
        if (done_) return;
        done_ = true;

        using namespace boost::json;
        object body;
        body["input_query"]= parse(input_);
        body["output_query"]= parse(output_raw);
        body["input_time"]= in_ts_;
        body["output_time"]= epoch_ms();

        curl_send(body);
    }

    ~LogTransaction()
    {
        if (!done_) {
            boost::json::object body;
            body["input_query"]= boost::json::parse(input_);
            body["input_time"]= in_ts_;
            body["output_time"]= epoch_ms();
            body["error"]= "Tx destroyed without complete()";
            curl_send(body);
        }
    }

    LogTransaction(LogTransaction const&)= delete;
    LogTransaction& operator=(LogTransaction const&)= delete;
    LogTransaction(LogTransaction&&)= default;
    LogTransaction& operator=(LogTransaction&&)= default;
};

}

