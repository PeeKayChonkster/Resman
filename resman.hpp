#ifndef RESMAN_HPP
#define RESMAN_HPP

#include <string>
#include <string.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace prim
{

namespace fs = std::filesystem;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    const std::string separator = "\\";
#else
    const std::string separator = "/";
#endif

#define MAX_RESFILES 1000u

struct Resfile
{
    std::string path;
    unsigned int size;
    std::string extension;
    unsigned char* data;

    Resfile(std::string p, unsigned int s, unsigned char* d): path(p), size(s), extension(fs::path(p).extension()), data(d) {}
    Resfile(const Resfile& other) = delete;
    Resfile(Resfile&& other)
    {
        path = other.path;
        other.path.clear();
        size = other.size;
        other.size = 0;
        extension = other.extension;
        other.extension = "";
        data = other.data;
        other.data = nullptr;
    }
    ~Resfile() 
    { 
        delete[] data; 
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
            data = other.data;
            other.data = nullptr;
            return *this;
        }
    }

    inline void print(std::ostream& stream) const
    {
        stream << "Path: " << path << std::endl;
        stream << "Size: " << size << std::endl;
        stream << "Extension: " << extension << std::endl;
        stream << "Data: \n" << data << std::endl;
    }
};

class Resman
{
private:
    inline static const std::string packageName = "resman_package.res";
    inline static std::string packagePath;
    inline static const std::string pathIdendifier = "%PATH";
    inline static std::vector<Resfile*> files;

    inline static bool isValidExtension(fs::path path)
    {
        // Those are forbidden extensions
        return path.extension() != ".res" && 
            path.extension() != ".cpp" && 
            path.extension() != ".hpp" &&
            path.extension() != ".exe" &&
            path.extension() != ".h"
            ;
    }

    static bool appendData(std::string path)
    {
        fs::path p(path);
        p.make_preferred();
        std::ifstream ifstream(p, std::ios::binary);
        if(!ifstream.good()) return false;
        auto iter = std::find_if(files.begin(), files.end(), [&p](const Resfile* file) { return file->path == p.string(); });
        // if data already in the datamap
        if(iter != files.end()) return false;
        std::stringstream ss;
        ss << ifstream.rdbuf();
        ifstream.close();
        std::string str(std::move(ss.str()));
        unsigned char* buf = new unsigned char[str.length()];
        std::copy(str.begin(), str.end(), buf);
        //! emplace in datamap isn't working for some reason
        files.push_back(new Resfile(p, str.length(), buf));
        return true;
    }
public:
    static bool packFolder(std::string folderPath, std::string outPath = ".")
    {
        outPath += separator + packageName;
        fs::path fPath(folderPath);
        fPath.make_preferred();
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

    static bool loadResourceFile(std::string path = ".")
    {
        path += separator + packageName;
        packagePath = path;
        std::ifstream ifstream(packagePath, std::ios::binary);
        if(!ifstream.good()) return false;
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
                        // change path separators accorging to the system
                        chunkPath = fs::path(chunkPath).make_preferred().string();
                        //! emplace in datamap isn't working for some reason
                        files.push_back(new Resfile(chunkPath, rawData.length(), buf));
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
        files.push_back(new Resfile(chunkPath, rawData.length(), buf));
        ifstream.close();

        // TEMP
        // for(auto it = data.begin(); it != data.end(); ++it)
        // {
        //     std::cout << "DATA PATHS" << std::endl;
        //     std::cout << it->first << std::endl;
        // }

        return true;
    }

    static Resfile* getFile(std::string path)
    {
        fs::path p(path);
        p.make_preferred();
        auto iter = std::find_if(files.begin(), files.end(), [&p](const Resfile* file) { return file->path == p.string(); });
        if(iter == files.end())
        {
            // didn't find data in datamap, search on disk
            if(!appendData(p))
            {
                // didn't find on disk
                std::cerr << "Didn't find file \"" << p << "\" in datamap nor on disk." << std::endl;
                return nullptr;
            }
            else
            {
                // found file on disk, appended it to the datamap
                //std::cout << "Found file on disk. Added to the datamap." << std::endl;
                auto iter = std::find_if(files.begin(), files.end(), [&p](const Resfile* file) { return file->path == p.string(); });
                return *iter;
            }
        }
        else
        {
            // file found in the datamap
            //std::cout << "Found file in the datamap." << std::endl;
            return *iter;
        }
    }

    /*
        * Call this to deallocate memory
    */
    static void free()
    {
        std::for_each(files.begin(), files.end(), [](Resfile* file){ delete file; });
    }
};
}

#endif // RESMAN_HPP