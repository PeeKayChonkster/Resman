#ifndef RESMAN_STATIC_HPP
#define RESMAN_STATIC_HPP

#include <string>
#include <string.h>
#include <optional>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <memory>

namespace chen
{

namespace fs = std::filesystem;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    const std::string separator = "\\";
#else
    const std::string separator = "/";
#endif

struct Resfile
{
    std::string path;
    unsigned int size;
    std::unique_ptr<unsigned char[]> data;

    Resfile(): Resfile("", 0, nullptr) {}
    Resfile(std::string p, unsigned int s, unsigned char* d): path(p), size(s), data(d) {}
    Resfile(const Resfile& other) = delete;
    Resfile(Resfile&& other)
    {
        path = other.path;
        other.path.clear();
        size = other.size;
        other.size = 0;
        data = std::move(other.data);
        other.data = nullptr;
    }
    Resfile& operator=(const Resfile& other) = delete;
    Resfile& operator=(Resfile&& other)
    {
        if(this != &other)
        {
            path = other.path;
            other.path.clear();
            size = other.size;
            other.size = 0;
            data = std::move(other.data);
            other.data = nullptr;
            return *this;
        }
    }

    inline void print(std::ostream& stream)
    {
        stream << "Path: " << path << std::endl;
        stream << "Size: " << size << std::endl;
        stream << "Data: \n" << data.get() << std::endl;
    }
};

class ResmanStatic
{
private:
    inline static const std::string packageName = "resman_package.res";
    static std::string packagePath;
    inline static const std::string pathIdendifier = "%PATH";
    inline static std::unordered_map<std::string, Resfile> data;

    inline static bool isValidExtension(fs::path path)
    {
        return path.extension() != ".res" && 
            path.extension() != ".cpp" && 
            path.extension() != ".hpp" &&
            path.extension() != ".exe" &&
            path.extension() != ".h"
            ;
    }

    static bool appendData(std::string path)
    {
        std::ifstream ifstream(path, std::ios::binary);
        if(!ifstream.good()) return false;
        auto iter = data.find(path);
        // if data already in the datamap
        if(iter != data.end()) return false;
        std::stringstream ss;
        ss << ifstream.rdbuf();
        ifstream.close();
        std::string str(std::move(ss.str()));
        unsigned char* buf = new unsigned char[str.length()];
        std::copy(str.begin(), str.end(), buf);
        //! emplace in datamap isn't working for some reason
        data.insert({ path, Resfile(path, str.length(), buf) });
        return true;
    }
public:
    static bool packFolder(std::string folderPath, std::string outPath = "." + separator + packageName)
    {
        fs::path fPath(folderPath);
        if(!fs::exists(fPath) || !fs::is_directory(fPath)) return false;
        packagePath = outPath;
        std::ofstream ofstream(outPath, std::ios::binary);
        if(!ofstream.good()) return false;
        std::ifstream ifstream;
        fs::recursive_directory_iterator iter(fPath);
        while(iter != fs::recursive_directory_iterator())
        {
            //  skip directories, executables(unix) and files with unwanted extensions
            if(!iter->is_directory() && fs::status(iter->path()).permissions() != fs::perms::owner_exec && isValidExtension(iter->path())) 
            {
                ifstream.open(iter->path().string(), std::ios::binary);
                ofstream << pathIdendifier << iter->path().string() << std::endl;
                ofstream << ifstream.rdbuf();
                ifstream.close();
            }
            ++iter;
        }
        ofstream.close();
        return true;
    }

    static bool loadResourceFile()
    {
        if(packagePath.empty()) return false;
        std::ifstream ifstream(packagePath, std::ios::binary);
        // char buffer
        char c = 0;
        std::string rawData;
        // path of current chunk
        std::string chunkPath;
        ifstream >> std::noskipws;
        while(ifstream >> c)
        {
            if(c == '%')
            {
                std::string tag;
                for(int i = 0; i < 4; ++i) 
                {
                    ifstream >> c;
                    tag.push_back(c);
                }
                if(tag == "PATH")
                {
                    // it's a path
                    if(!chunkPath.empty())
                    {
                        // write current chunk to the datamap
                        unsigned char* buf = new unsigned char[rawData.length()];
                        std::copy(rawData.begin(), rawData.end(), buf);
                        //! emplace in datamap isn't working for some reason
                        data.insert({ chunkPath, Resfile(chunkPath, rawData.length(), buf) });
                        chunkPath.clear();
                    }
                    // clear chunk buffer
                    rawData.clear();
                    ifstream >> c;
                    while(c != '\n')
                    {
                        // read new chunk path
                        chunkPath.push_back(c);
                        ifstream >> c;
                    }
                    continue;
                }
                else
                {
                    // bring file pointer 4 chars back
                    ifstream.seekg(-4, std::ios::cur);
                }
            }
            rawData.push_back(c);
        }
        // insert last chunk
        unsigned char* buf = new unsigned char[rawData.length()];
        std::copy(rawData.begin(), rawData.end(), buf);
        data.insert({ chunkPath, Resfile(chunkPath, rawData.length(), buf) });
        ifstream.close();
        return true;
    }

    static Resfile* getFile(std::string path)
    {
        fs::path p(path);
        p.make_preferred();
        path = p.string();
        auto iter = data.find(path);
        if(iter == data.end())
        {
            // didn't find data in datamap, search on disk
            if(!appendData(path))
            {
                // didn't find on disk OR file already exists in datamap
                std::cerr << "Didn't find file \"" << path << "\" in datamap nor on disk." << std::endl;
                return nullptr;
            }
            else
            {
                // found file on disk, appended it to the datamap
                //std::cout << "Found file on disk. Added to the datamap." << std::endl;
                auto iter = data.find(path);
                return &iter->second;
            }
        }
        else
        {
            // file found in the datamap
            //std::cout << "Found file in the datamap." << std::endl;
            return &iter->second;
        }
    }
};
}

#endif // RESMAN_STATIC_HPP