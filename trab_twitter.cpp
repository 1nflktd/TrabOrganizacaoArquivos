// bibliotecas padrao
#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <experimental/optional>
// bibliotecas curl
#include <curl/curl.h>
// bibliotecas bsoncxx
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/json.hpp>
// bibliotecas mongodb
#include <mongocxx/client.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/instance.hpp>

#define TAMANHO_LINHA_ARQ	89
#define SUBSTR_CHAVE		0, 7
#define SUBSTR_NOME			7, 45
#define SUBSTR_AUTOR		52, 30
#define SUBSTR_CODIGO		82, 7

/******************************************/

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

/******************************************/


using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

enum CAMPO_ARQ
{
    CHAVE = 1,
	NOME,
	AUTOR,
	CODIGO
};

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

std::string trim(const std::string& str,
                 const std::string& whitespace = " ")
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

bool inserirLivrosBD(const mongocxx::v0::database & db)
{
	std::ifstream linhas ("livrosELC.txt");
	if (linhas.fail())
	{
		return false;
	}

	std::cout << "Carregando.......espere.....\n";

	std::string linha;
	while (getline(linhas, linha))
	{
		std::string nome_livro{trim(linha.substr(SUBSTR_NOME))};
		try
		{
			auto tweets = obterTweets(nome_livro);
			if (tweets)
			{
				auto livro_doc = document{}
					<< "chave" << std::atoi(linha.substr(SUBSTR_CHAVE).c_str())
					<< "nome" << nome_livro
					<< "autor" << trim(linha.substr(SUBSTR_AUTOR))
					<< "codigo" << std::atoi(linha.substr(SUBSTR_CODIGO).c_str())
					<< "tweets" << bsoncxx::types::b_array{tweets.value()}
					<< finalize;
				auto res = db["livros"].insert_one(livro_doc);
			}
			else
			{
				std::cout << "entrou em baixo\n";
				auto livro_doc = document{}
					<< "chave" << std::atoi(linha.substr(SUBSTR_CHAVE).c_str())
					<< "nome" << nome_livro
					<< "autor" << trim(linha.substr(SUBSTR_AUTOR))
					<< "codigo" << std::atoi(linha.substr(SUBSTR_CODIGO).c_str())
					<< "tweets" << bsoncxx::builder::stream::open_array << bsoncxx::builder::stream::close_array
					<< finalize;
				auto res = db["livros"].insert_one(livro_doc);
			}
		}
		catch (const std::runtime_error& e){ std::cout << e.what() << "\n"; }
	}

	return true;
}

template <typename T>
void consultar(const mongocxx::v0::database & db, std::string campo, T valor)
{
	document filter;
	filter << campo << valor;
	
	auto cursor = db["livros"].find(filter);
	bool achou = false;
	for (auto&& doc : cursor)
	{
		std::cout << bsoncxx::to_json(doc) << std::endl;
		achou = true;
	}

	if (!achou)
	{
		std::cout << "Nenhum resultado encontrado!\n";
	}
}

void showMenu()
{
	std::cout << "|----------------------------------|\n";
	std::cout << "| Pesquisar por:                   |\n";
	std::cout << "| 1 - Chave                        |\n";
	std::cout << "| 2 - Nome do livro                |\n";
	std::cout << "| 3 - Autor                        |\n";
	std::cout << "| 4 - Codigo                       |\n";
	std::cout << "|----------------------------------|\n";
	std::cout << "| 0 - Sair                         |\n";
	std::cout << "|----------------------------------|\n";
}

int main()
{
	mongocxx::instance inst{};
	mongocxx::client conn{mongocxx::uri{}};
	
	auto db = conn["organizacao"];

	bool livrosCarregados = false;
	int inputUsuario;
	do 
	{
		showMenu();
		std::cout << "Escolha uma opcao: ";
		if(!(std::cin >> inputUsuario))
		{
			std::cout << "Numeros somente!\n";
		}
		std::cin.clear();
      	std::cin.ignore(10000,'\n');

		if (inputUsuario != 0) 
		{
			if (!livrosCarregados)
			{
				livrosCarregados = inserirLivrosBD(std::move(db));
			}

	        int valorInt;
	        std::string valorStr;

	        std::cout << "Digite sua consulta: ";

			switch(inputUsuario)
			{
				case CODIGO:
					std::cin >> valorInt;
					consultar<int>(std::move(db), "codigo", valorInt);
					break;
	    		case CHAVE:
					std::cin >> valorInt;
	                consultar<int>(std::move(db), "chave", valorInt);
	                break;
				case NOME:
					std::getline(std::cin, valorStr);
					valorStr = trim(valorStr);
					consultar<std::string>(std::move(db), "nome", valorStr);
	                break;
				case AUTOR:
					std::getline(std::cin, valorStr);
					valorStr = trim(valorStr);
	                consultar<std::string>(std::move(db), "autor", valorStr);
					break;
				default:
					std::cout << "Digite um valor entre 0 e 4!\n";
					inputUsuario = 9;
					break;
			}
		}
	} 
	while (inputUsuario > 0);
	
	//db["livros"].drop();

	return 0;
}
