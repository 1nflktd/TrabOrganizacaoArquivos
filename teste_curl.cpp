#include <iostream>
#include <map>
#include <string>
#include <experimental/optional>
#include <curl/curl.h>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/array/view.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/stdx/string_view.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/types/value.hpp>

typedef std::map<std::string, std::string> headermap;

/** response struct for queries */
typedef struct
{
	int code;
	std::string body;
	headermap headers;
} response;


/**
 * @brief write callback function for libcurl
 *
 * @param data returned data of size (size*nmemb)
 * @param size size parameter
 * @param nmemb memblock parameter
 * @param userdata pointer to user data to save/work with return data
 *
 * @return (size * nmemb)
 */
size_t write_callback(void *data, size_t size, size_t nmemb,
                            void *userdata)
{
	response* r;
	r = reinterpret_cast<response*>(userdata);
	r->body.append(reinterpret_cast<char*>(data), size*nmemb);

	return (size * nmemb);
}

/**
 * @brief header callback for libcurl
 *
 * @param data returned (header line)
 * @param size of data
 * @param nmemb memblock
 * @param userdata pointer to user data object to save headr data
 * @return size * nmemb;
 */
size_t header_callback(void *data, size_t size, size_t nmemb,
                            void *userdata)
{
	response* r;
	r = reinterpret_cast<response*>(userdata);
	std::string header(reinterpret_cast<char*>(data), size*nmemb);
	size_t seperator = header.find_first_of(":");
	if ( std::string::npos == seperator )
	{
		//roll with non seperated headers...
		//trim(header);
		if ( 0 == header.length() )
		{
			return (size * nmemb); //blank line;
		}
		r->headers[header] = "present";
	}
	else
	{
		std::string key = header.substr(0, seperator);
		//trim(key);
		std::string value = header.substr(seperator + 1);
		//trim (value);
		r->headers[key] = value;
	}

	return (size * nmemb);
}

std::experimental::optional<bsoncxx::array::value> obterTweets (std::string consulta) 
{
	CURL * curl = NULL;
	CURLcode res;

	response ret = {};

	curl = curl_easy_init();
	if(curl)
	{
		struct curl_slist *chunk = NULL;
		
		char * url_codificada = curl_easy_escape(curl, consulta.c_str(), consulta.length());
		std::string url_codificada_str {url_codificada};
		std::string url {"https://api.twitter.com/1.1/search/tweets.json?q="};
		url += url_codificada_str;

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // "https://api.twitter.com/1.1/search/tweets.json?q=dostoievski+karamazov+irmaos");

		chunk = curl_slist_append(chunk, "Authorization:Bearer AAAAAAAAAAAAAAAAAAAAALSuigAAAAAAigtciMNxS%2FFHBu3aLnYRwKHc4X8%3DRSJi7DonxsBWdZSHBCmGm16PFepLzI0KynRs6grFWzURi1Xs1B");

		res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &ret);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			ret.body = "Failed to query.";
			ret.code = -1;
			//return ret;
			return std::experimental::nullopt;
		}

		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		ret.code = static_cast<int>(http_code);

	    curl_easy_cleanup(curl);
	    curl_global_cleanup();

		bsoncxx::stdx::optional<bsoncxx::document::value> o1 = bsoncxx::from_json(ret.body);

		bsoncxx::document::value o2 = o1.value();
		bsoncxx::document::view o3 = o2.view();
		for (bsoncxx::document::element ele : o3)
		{
			if(ele.type() == bsoncxx::type::k_array)
			{
			    bsoncxx::array::view subarr{ele.get_array().value};
				
				auto arr = bsoncxx::builder::stream::array{};
				
			    for (bsoncxx::array::element ele2 : subarr)
				{
					if (!ele2) break;

					bsoncxx::types::b_document x = ele2.get_document();
										
					bsoncxx::document::view y = x.value;
					bsoncxx::document::element usr = y["user"];
					bsoncxx::types::b_document usrx = usr.get_document();
					bsoncxx::document::view usrxx = usrx.value;

					auto docTxtUsr = bsoncxx::builder::stream::document{}
						<< "text" << y["text"].get_value() 
						<< "user" << usrxx["name"].get_value()
					<< bsoncxx::builder::stream::finalize;

					arr << bsoncxx::types::b_document{docTxtUsr};
			    }
				auto docFinal = bsoncxx::builder::stream::array{}
						 << bsoncxx::types::b_array{arr}
						 << bsoncxx::builder::stream::finalize;
				return docFinal;
			}
		}
	}
	return std::experimental::nullopt;
}

int main()
{
	auto doc = obterTweets(std::string{"liverpool"});
	if (doc)
	{
		auto docX = bsoncxx::builder::stream::document{}
			<< "eu" << "çaça"
			<< "tweets" 
			<< bsoncxx::types::b_array{doc.value()}
		<< bsoncxx::builder::stream::finalize;
		std::cout << bsoncxx::to_json(docX);
	}
	
	// linha compilacao
	// g++ teste_curl.cpp -o teste_curl -std=c++14 `pkg-config --cflags --libs libmongocxx` -lcurl
	return 0;
}
