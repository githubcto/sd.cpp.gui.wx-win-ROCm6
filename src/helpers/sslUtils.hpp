#ifndef SSLUTILS_H
#define SSLUTILS_H
#include <openssl/evp.h>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>

namespace sd_gui_utils {
    inline std::string calculateHash(const std::string& filePath, const EVP_MD* hashAlgorithm) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Nem sikerült megnyitni a fájlt: " + filePath);
        }

        // Hash kontextus inicializálása
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(ctx, hashAlgorithm, nullptr);

        // Buffer az olvasáshoz
        std::vector<unsigned char> buffer(4096);
        while (file.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || file.gcount() > 0) {
            EVP_DigestUpdate(ctx, buffer.data(), file.gcount());
        }

        // Hash kiolvasása
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int length = 0;
        EVP_DigestFinal_ex(ctx, hash, &length);
        EVP_MD_CTX_free(ctx);

        // Hash hexadecimális ábrázolása
        std::ostringstream hashString;
        for (unsigned int i = 0; i < length; ++i) {
            hashString << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return hashString.str();
    }

    /**
     * Calculates the MD5 hash of a file.
     *
     * @param filePath path to the file to hash
     * @return the MD5 hash of the file as a hexadecimal string
     * @throws std::runtime_error if the file cannot be opened
     */
    inline std::string calculateMD5(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Nem sikerült megnyitni a fájlt: " + filePath);
        }

        const auto hashAlgorithm = EVP_md5();

        // Hash kontextus inicializálása
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(ctx, hashAlgorithm, nullptr);

        // Buffer az olvasáshoz
        std::vector<unsigned char> buffer(4096);
        while (file.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || file.gcount() > 0) {
            EVP_DigestUpdate(ctx, buffer.data(), file.gcount());
        }

        // Hash kiolvasása
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int length = 0;
        EVP_DigestFinal_ex(ctx, hash, &length);
        EVP_MD_CTX_free(ctx);

        // Hash hexadecimális ábrázolása
        std::ostringstream hashString;
        for (unsigned int i = 0; i < length; ++i) {
            hashString << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return hashString.str();
    }

    inline std::string sha256_string_openssl(const std::string& input) {
        std::stringstream ss;
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        const EVP_MD* md  = EVP_sha256();
        EVP_DigestInit_ex(mdctx, md, NULL);
        EVP_DigestUpdate(mdctx, input.data(), input.length());
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hashLength;
        EVP_DigestFinal_ex(mdctx, hash, &hashLength);
        for (unsigned int i = 0; i < hashLength; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        EVP_MD_CTX_free(mdctx);
        return ss.str();
    }

    inline std::string sha256_file_openssl(const char* path, void* custom_pointer, void (*callback)(size_t, std::string, void* custom_pointer)) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return "";
        }

        std::streamsize bufferSize = 1 * 1024 * 1024;
        char* buffer               = new char[bufferSize];

        std::string hashResult;
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hashLength;
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        const EVP_MD* md  = EVP_sha256();
        EVP_DigestInit_ex(mdctx, md, NULL);
        callback(file.tellg(), hashResult, custom_pointer);

        while (file.good()) {
            file.read(buffer, bufferSize);
            std::streamsize bytesRead = file.gcount();
            EVP_DigestUpdate(mdctx, buffer, bytesRead);

            if (file.tellg() % (20 * 1024 * 1024) == 0) {
                callback(file.tellg(), "", custom_pointer);
            }
        }

        EVP_DigestFinal_ex(mdctx, hash, &hashLength);
        EVP_MD_CTX_free(mdctx);

        std::stringstream ss;
        for (unsigned int i = 0; i < hashLength; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        callback(std::filesystem::file_size(path), ss.str(), custom_pointer);

        delete[] buffer;
        return ss.str();
    }
}  // namespace sd_gui_utils
#endif  // SSLUTILS_H