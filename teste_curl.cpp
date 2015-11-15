#include <iostream>
#include <map>
#include <string>
#include <curl/curl.h>

#include <bsoncxx/array/view.hpp>
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

int main()
{
	// pegar bearer token
	// https://api.twitter.com/oauth2/token
	CURL * curl = NULL;
	CURLcode res;

	response ret = {};

	curl = curl_easy_init();
	if(curl)
	{
		struct curl_slist *chunk = NULL;

		curl_easy_setopt(curl, CURLOPT_URL, "https://api.twitter.com/1.1/search/tweets.json?q=dostoievski+karamazov+irmaos");

		chunk = curl_slist_append(chunk, "Authorization:Bearer AAAAAAAAAAAAAAAAAAAAALSuigAAAAAAigtciMNxS%2FFHBu3aLnYRwKHc4X8%3DRSJi7DonxsBWdZSHBCmGm16PFepLzI0KynRs6grFWzURi1Xs1B");

		res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		/** set data object to pass to callback function */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
		/** set the header callback function */
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
		/** callback object for headers */
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &ret);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			ret.body = "Failed to query.";
			ret.code = -1;
			//return ret;
			return -1;
		}

		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		ret.code = static_cast<int>(http_code);

	    curl_easy_cleanup(curl);
	    curl_global_cleanup();

		bsoncxx::stdx::optional<bsoncxx::document::value> o1 = bsoncxx::from_json(ret.body);
		//auto o2 = o1.value();
		bsoncxx::document::value o2 = o1.value();
		bsoncxx::document::view o3 = o2.view();
		for (bsoncxx::document::element ele : o3)
		{
		    // element is non owning view of a key-value pair within a document.

		    // we can use the key() method to get a string_view of the key.
		    bsoncxx::stdx::string_view field_key{ele.key()};

		    std::cout << "Got key, key = " << field_key << std::endl;

		    // we can use type() to get the type of the value.
		    switch (ele.type()) {
		    case bsoncxx::type::k_utf8:
		        std::cout << "Got String!" << std::endl;
		        break;
		    case bsoncxx::type::k_oid:
		        std::cout << "Got ObjectId!" << std::endl;
		        break;
		    case bsoncxx::type::k_array: {
		        std::cout << "Got Array!" << std::endl;
		        // if we have a subarray, we can access it by getting a view of it.
		        bsoncxx::array::view subarr{ele.get_array().value};

		        for (bsoncxx::array::element ele2 : subarr) {
					if(ele2.type() == bsoncxx::type::k_array)
					{
						bsoncxx::array::view tset = ele2.get_array().value;
						for(auto ele3 : tset)
						{
							//bsoncxx::document::view ttt = ele3.get_value();
							//std::cout << "Text\n" << ele3.get_value()["text"] << "\n";
							std::cout << "ELE3\n" << bsoncxx::to_json(ele3.get_value()) << "\n";
						}
				        //std::cout << "array element: " << bsoncxx::to_json(ele2.get_value()) << std::endl;
					}
					else if (ele2.type() == bsoncxx::type::k_utf8)
					{
						std::cout << "Ele2\n" << bsoncxx::to_json(ele2.get_value()) << "\n";
					}
					else
					{
						std::cout << "something\n";
						/*
						bsoncxx::stdx::string_view t1 = bsoncxx::to_json(ele2.get_value());
						bsoncxx::document::value value (reinterpret_cast<const uint8_t*>(t1.data()), t1.size(), bson_free_deleter);
						bsoncxx::document::view view = value.view();
						std::cout << bsoncxx::to_json(view["text"]) << "\n";
						*/
						
						bsoncxx::stdx::string_view t1 = "{\"metadata\" : {\"iso_language_code\" : \"pt\",\"result_type\" : \"recent\" }, \"text\" : \"xxxãâøððæßð/?€/?€©«©»©“““““““““ßßððđ\" }";
						bsoncxx::stdx::optional<bsoncxx::document::value> o11 = bsoncxx::from_json(t1);

						//bsoncxx::stdx::optional<bsoncxx::document::value> o11 = bsoncxx::from_json(bsoncxx::to_json(ele2.get_value()));
						if(o11)
						{
							bsoncxx::document::value o21 = o11.value();
							bsoncxx::document::view o31 = o21.view();
							std::cout << bsoncxx::to_json(o31["text"]) << "\n";
						}
						//std::cout << bsoncxx::to_json(ele2.get_value()) << "\n";
					}
		        }
		        break;
		    }
		    default:
		        std::cout << "We messed up!" << std::endl;
	  	  }

		    // usually we don't need to actually use a switch statement, because we can also
		    // get a variant 'value' that can hold any BSON type.
		    //bsoncxx::types::value ele_val{ele.get_value()};
		    // if we need to print an arbitrary value, we can use to_json, which provides
		    // a suitable overload.
		    //std::cout << "the value is " << bsoncxx::to_json(ele_val) << std::endl;;


		}
		//auto statuses = o3["statuses"].get_array().view()[0]["text"];

		//std::cout << bsoncxx::to_json() << "\n";
		/*
		auto arr = statuses.get_array();
		for (auto && doc : arr)
		{
			std::cout << bsoncxx::to_json(doc) << "\n";
		}
		*/

		//std::cout << ret.body << "\n";

	}



	// linha compilacao
	// g++ teste_curl.cpp -o teste_curl -std=c++11 `pkg-config --cflags --libs libmongocxx` -lcurl
	return 0;
}
