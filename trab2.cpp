// bibliotecas padrao
#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <string>
#include <cstdlib>
#include <algorithm>
// bibliotecas mongodb
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/instance.hpp>

#define TAMANHO_LINHA_ARQ	89
#define SUBSTR_CHAVE		0, 7
#define SUBSTR_NOME			7, 45
#define SUBSTR_AUTOR		52, 30
#define SUBSTR_CODIGO		82, 7

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

enum CAMPO_ARQ
{
    CHAVE = 1,
	NOME,
	AUTOR,
	CODIGO
};

struct livro
{
	int chave;
	std::string nome;
	std::string autor;
	int codigo;
public:
    livro() :
        chave(-1), nome(std::string()), autor(std::string()), codigo(-1)
    {}
    
    livro(int _chave, std::string _nome, std::string _autor, int _codigo) : 
        chave(_chave), nome(_nome), autor(_autor), codigo(_codigo)
    {}
    
};

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
		auto livro_doc = document{}
			<< "chave" << std::atoi(linha.substr(SUBSTR_CHAVE).c_str())
			<< "nome" << trim(linha.substr(SUBSTR_NOME))
			<< "autor" << trim(linha.substr(SUBSTR_AUTOR))
			<< "codigo" << std::atoi(linha.substr(SUBSTR_CODIGO).c_str())
			<< finalize;

		auto res = db["livros"].insert_one(livro_doc);
	}

	return true;
}

template <typename T>
std::vector<livro> consultar(const mongocxx::v0::database & db, std::string campo, T valor)
{
	std::vector<livro> vetorResultado;
	
	document filter;
	filter << campo << valor;
	
	auto cursor = db["livros"].find(filter);
	for (auto&& doc : cursor)
	{
		//std::cout << bsoncxx::to_json(doc) << std::endl;
		
		livro l(
			doc["chave"].get_int32(),
			doc["nome"].get_utf8(),
			doc["autor"].get_utf8(),
			doc["codigo"].get_int32()
		);
		vetorResultado.push_back(l);
	}

	return vetorResultado;
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

void showResultados(const std::vector<livro> & resultados)
{
	std::cout << "Resultados da consulta:\n";
	for (auto & r : resultados)
	{
		std::cout << "Chave: " << r.chave << " Nome: " << r.nome << " Autor: " << r.autor << " Codigo: " << r.codigo << "\n";
	}
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
				//livrosCarregados = inserirLivrosBD(std::move(db));
			}

			std::vector<livro> resultados;
	        int valorInt;
	        std::string valorStr;

	        std::cout << "Digite sua consulta: ";

			switch(inputUsuario)
			{
				case CODIGO:
					std::cin >> valorInt;
					resultados = consultar<int>(std::move(db), "codigo", valorInt);
					break;
	    		case CHAVE:
					std::cin >> valorInt;
	                resultados = consultar<int>(std::move(db), "chave", valorInt);
	                break;
				case NOME:
					std::getline(std::cin, valorStr);
					valorStr = trim(valorStr);
					resultados = consultar<std::string>(std::move(db), "nome", valorStr);
	                break;
				case AUTOR:
					std::getline(std::cin, valorStr);
					valorStr = trim(valorStr);
	                resultados = consultar<std::string>(std::move(db), "autor", valorStr);
					break;
				default:
					std::cout << "Digite um valor entre 0 e 4!\n";
					inputUsuario = 9;
					break;
			}

			if (inputUsuario != 9)
			{
				if(resultados.empty())
					std::cout << "Nenhum resultado encontrado!\n";
				else
					showResultados(resultados);
			}
		}
	} 
	while (inputUsuario > 0);
	
	//db["livros"].drop();

	return 0;
}
