#ifndef SHADER_UTILS_HPP
#define SHADER_UTILS_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

namespace ShaderUtils {

    // Função para ler o conteúdo de um arquivo de shader e retorná-lo como uma string
    inline std::string readShaderFile(const char* filePath) {
        std::ifstream shaderFile;
        // Garante que ifstream pode lançar exceções em caso de falha
        shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            // Abre o arquivo
            shaderFile.open(filePath);

            // Cria um stringstream para ler o buffer do arquivo
            std::stringstream shaderStream;
            shaderStream << shaderFile.rdbuf();

            // Fecha o manipulador de arquivo
            shaderFile.close();

            // Converte o stream em uma string e a retorna
            return shaderStream.str();
        } catch (std::ifstream::failure& e) {
            // Em caso de erro, exibe uma mensagem
            std::cerr << "ERRO::SHADER::ARQUIVO_NAO_LIDO_COM_SUCESSO: " << filePath << "\n" << e.what() << std::endl;
            return ""; // Retorna uma string vazia em caso de erro
        }
    }

} // namespace ShaderUtils

#endif // SHADER_UTILS_HPP