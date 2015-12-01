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

enum CAMPO_ARQ
{
    CHAVE = 1,
	NOME,
	AUTOR,
	CODIGO,
	USER,
	TEXT
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

		auto livro_doc = bsoncxx::builder::stream::document{}
			<< "chave" << std::atoi(linha.substr(SUBSTR_CHAVE).c_str())
			<< "nome" << nome_livro
			<< "autor" << trim(linha.substr(SUBSTR_AUTOR))
			<< "codigo" << std::atoi(linha.substr(SUBSTR_CODIGO).c_str())
			<< "carregouTweets" << false
			<< bsoncxx::builder::stream::finalize;

		//std::cout << "Carregou " << bsoncxx::to_json(livro_doc) << "\n";

		auto res = db["livros"].insert_one(livro_doc);

	}

	return true;
}

bool carregarTweets(const mongocxx::v0::database & db)
{
	bsoncxx::builder::stream::document filter;
	
	filter << "carregouTweets" << false;

	auto cursor = db["livros"].find(filter);

	for (bsoncxx::document::view docview : cursor)
	{
		bool twitter_ok = true;
		do
		{
			try
			{
				std::string nome_livro {docview["nome"].get_utf8().value};
				auto tweets = obterTweets(nome_livro);
				if (tweets)
				{
					bsoncxx::builder::stream::document filterUpdate;
					filterUpdate << "chave" << docview["chave"].get_value();
					bsoncxx::builder::stream::document update;
					update << "$set" << bsoncxx::builder::stream::open_document
							<< "tweets" << bsoncxx::types::b_array{tweets.value()}
							<< "carregouTweets" << true
							<< bsoncxx::builder::stream::close_document;
					auto res = db["livros"].update_one(filterUpdate, update);
					twitter_ok = true;
				}
			}
			catch (const std::runtime_error& e)
			{
				////std::cout << e.what() << "\n";
				std::cout << "twitter bloqueado\n";
				twitter_ok = false;
			}
			catch (const std::exception & e)
			{
				std::cout << "exception query\n";
				std::cout << e.what() << "\n";
				twitter_ok = false;
			}
		}
		while(!twitter_ok);
	}

	return true;
}

template <typename T>
void consultar(const mongocxx::v0::database & db, std::string campo, T valor)
{
	bsoncxx::builder::stream::document filter;

	if (campo == "user" || campo == "text")
	{
		std::string consulta {"tweets.0."};
		consulta += campo;
		if(campo == "user")
			filter << consulta << valor;
		else 
			filter << consulta << bsoncxx::builder::stream::open_document << "$regex" << valor << bsoncxx::builder::stream::close_document;
		
	}
	else
	{
		filter << campo << valor;
	}
	
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
	std::cout << "| 5 - Usuário                      |\n";
	std::cout << "| 6 - Tweet                        |\n";
	std::cout << "|----------------------------------|\n";
	std::cout << "| 7 - Carregar livros              |\n";
	std::cout << "| 8 - Carregar tweets              |\n";
	std::cout << "|----------------------------------|\n";
	std::cout << "| 0 - Sair                         |\n";
	std::cout << "|----------------------------------|\n";
}

int main()
{
	mongocxx::instance inst{};
	mongocxx::client conn{mongocxx::uri{}};
	
	auto db = conn["organizacao"];

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
			if (inputUsuario == 7 || inputUsuario == 8)
			{
				if(inputUsuario == 7)
				{
					if(!inserirLivrosBD(std::move(db)))
						std::cout << "Erro ao carregar os livros\n";
				}
				else
				{
					if(!carregarTweets(std::move(db)))
						std::cout << "Erro ao carregar os tweets\n";
				}
			}
			else
			{

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
					case USER:
						std::getline(std::cin, valorStr);
						valorStr = trim(valorStr);
			            consultar<std::string>(std::move(db), "user", valorStr);
						break;
					case TEXT:
						std::getline(std::cin, valorStr);
						valorStr = trim(valorStr);
			            consultar<std::string>(std::move(db), "text", valorStr);
						break;
					default:
						std::cout << "Digite um valor entre 0 e 6!\n";
						inputUsuario = 9;
						break;
				}
			}
		}
	} 
	while (inputUsuario > 0);
	
	//db["livros"].drop();

	return 0;
}
