#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <string>
#include <cstdlib>
#include <algorithm>

#define TAMANHO_LINHA_ARQ	89
#define SUBSTR_CHAVE		0, 7
#define SUBSTR_NOME		7, 45
#define SUBSTR_AUTOR		52, 30
#define SUBSTR_CODIGO		82, 7

enum CAMPO_ARQ
{
    CHAVE = 1,
	NOME,
	AUTOR,
	CODIGO
};

template <typename T>
struct indice
{
	int linha;
	T campo;

public:
	indice(T _c) : linha(-1), campo(_c) {}
	indice(int _l, T _c) : linha(_l), campo(_c) {}
};

template <typename T>
struct compara
{
    bool operator() (const indice<T> & lhs, const indice<T> &rhs) { return lhs.campo < rhs.campo; }
};

struct livro
{
	int chave;
	std::string nome;
	std::string autor;
	int codigo;
	int linha;
public:
    livro() :
        chave(-1), nome(std::string()), autor(std::string()), codigo(-1), linha(-1)
    {}
    
    livro(int _chave, std::string _nome, std::string _autor, int _codigo, int _linha) : 
            chave(_chave), nome(_nome), autor(_autor), codigo(_codigo), linha(_linha)
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

bool carregarIndices(
		std::multiset<indice<int>, compara<int>> & indiceChave, 
		std::multiset<indice<std::string>, compara<std::string>> & indiceNome,
		std::multiset<indice<std::string>, compara<std::string>> & indiceAutor,
		std::multiset<indice<int>, compara<int>> & indiceCodigo
	)
{
	std::ifstream linhas ("livrosELC.txt");
	if (linhas.fail())
	{
		return false;
	}

	std::cout << "Carregando.......espere.....\n";

	std::string linha;
	int nroLinha = 0;
	while (getline(linhas, linha))
	{
		indice<int> l1(nroLinha, std::atoi(linha.substr(SUBSTR_CHAVE).c_str()));
		indiceChave.emplace(l1);

		indice<std::string> l2(nroLinha, trim(linha.substr(SUBSTR_NOME)));
		indiceNome.emplace(l2);

		indice<std::string> l3(nroLinha, trim(linha.substr(SUBSTR_AUTOR)));
		indiceAutor.emplace(l3);

		indice<int> l4(nroLinha, std::atoi(linha.substr(SUBSTR_CODIGO).c_str()));
		indiceCodigo.emplace(l4);

		nroLinha++;
	}

	return true;
}

template <typename T>
std::vector<livro> consultar(std::multiset<indice<T>, compara<T>> indiceConsulta, T valor)
{
	std::vector<livro> vetorResultado;

	std::ifstream linhas ("livrosELC.txt");
	if (linhas.fail())
	{
		return vetorResultado;
	}

	for(auto & i : indiceConsulta)
	{
		if (i.campo == valor)
		{
			linhas.seekg(i.linha * TAMANHO_LINHA_ARQ);
			std::string linhaArq;
			if (getline(linhas, linhaArq))
			{
		        livro l(
		            std::atoi(linhaArq.substr(SUBSTR_CHAVE).c_str()),
		            trim(linhaArq.substr(SUBSTR_NOME)),
		            trim(linhaArq.substr(SUBSTR_AUTOR)),
		            std::atoi(linhaArq.substr(SUBSTR_CODIGO).c_str()),
		            i.linha
		        );
		        vetorResultado.push_back(l);
			}
		}
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

void showResultados(std::vector<livro> resultados)
{
	std::cout << "Resultados da consulta:\n";
	for (auto & r : resultados)
	{
		std::cout << "Chave: " << r.chave << " Nome: " << r.nome << " Autor: " << r.autor << " Codigo: " << r.codigo << "\n";
	}
}

int main()
{
	bool indicesCarregados = false;


    std::multiset<indice<int>, compara<int>> 					indiceChave;
	std::multiset<indice<std::string>, compara<std::string>> 	indiceNome;
	std::multiset<indice<std::string>, compara<std::string>> 	indiceAutor;
	std::multiset<indice<int>, compara<int>>            		indiceCodigo;

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
			if (!indicesCarregados)
			{
				indicesCarregados = carregarIndices(indiceChave, indiceNome, indiceAutor, indiceCodigo);
			}

			std::vector<livro> resultados;
	        int valorInt;
	        std::string valorStr;

	        std::cout << "Digite sua consulta: ";

			switch(inputUsuario)
			{
				case CODIGO:
					std::cin >> valorInt;
					resultados = consultar<int>(indiceCodigo, valorInt);
					break;
	    		case CHAVE:
					std::cin >> valorInt;
	                resultados = consultar<int>(indiceChave, valorInt);
	                break;
				case NOME:
					std::getline(std::cin, valorStr);
					valorStr = trim(valorStr);
					resultados = consultar<std::string>(indiceNome, valorStr);
	                break;
				case AUTOR:
					std::getline(std::cin, valorStr);
					valorStr = trim(valorStr);
	                resultados = consultar<std::string>(indiceAutor, valorStr);
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

	return 0;
}
