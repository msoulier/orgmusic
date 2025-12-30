#define _UNICODE
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>
#include <filesystem>
#include <vector>
#include <map>
#include <stdio.h>
#include <cassert>
#include <algorithm>

#include <MediaInfo/MediaInfo.h>

#include "config.h"

namespace fs = std::filesystem;

static bool genre = false;
static std::vector<std::string> input_files;
static std::string outdir;
static std::map<std::string, int> genremap;

int
process_file(fs::path path)
{
    std::string spath = path.string();
    int rv = 0;
    // MediaInfo expects unicode wchar_t*
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring myinput = converter.from_bytes(spath);

    MediaInfoLib::MediaInfo info;
    if (info.Open(myinput) == 0) {
        std::cerr << "ERROR: file not opened: " << spath << std::endl;
    } else {
        MediaInfoLib::String wdetails = info.Inform();
        std::string details = converter.to_bytes(wdetails);
        //std::cout << details << std::endl;
        if (genre) {
            std::wstring genre = info.Get(MediaInfoLib::Stream_General, 0, L"Genre");
            // convert back
            std::string genre_str = converter.to_bytes(genre);
            if (genre_str.empty()) {
                std::cerr << "Unknown Genre: " << spath << std::endl;
            } else {
                //std::cout << genre_str << std::endl;
                if (genremap.find(genre_str) == genremap.end()) {
                    genremap[genre_str] = 0;
                }
                genremap[genre_str] += 1;
            }
        }
        info.Close();
        rv = 1;
    }
    return rv;
}

int
parse_arguments(int argc, char *argv[])
{
    int options = 0;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if ((arg == "--genre") || (arg == "-r")) {
            genre = true;
            options++;
        } else {
            if (i == argc) {
                // last argument, if no input files then it's input
                if (input_files.empty()) {
                    input_files.push_back(arg);
                } else {
                    outdir = arg;
                }
            } else {
                input_files.push_back(arg);
            }
        }
    }
    if (input_files.empty()) {
        return -1;
    } else {
        return options;
    }
}

void
report()
{
    if (genre) {
        // Vector to hold sorted pairs
        std::vector<std::pair<std::string, int>> sortvec(genremap.begin(), genremap.end());

        // Sort by values.
        std::sort(sortvec.begin(), sortvec.end(), 
            [](const auto& a, const auto& b) {
                // return a.second < b.second;  // ascending
                return a.second > b.second;  // for descending
            });

        std::cout << "Genre report:" << std::endl;
        for (const auto& [key, value] : sortvec) {
            printf("%20s    %5d\n", key.c_str(), value);
        }
    }
}

int
main(int argc, char *argv[])
{
    int options = 0;
    if ((options = parse_arguments(argc, argv)) < 0)
    {
        std::cerr << "Usage: orgmusic [--genre] <input paths> [output path]" << std::endl;
        exit(1);
    }
    assert( argc >= 3 );
    for (int i = options+1; i < argc; ++i)
    {
        fs::path path(argv[i]);
        //std::cout << i << ": input path: " << path << std::endl;
        fs::file_status status = fs::status(path);
        if (fs::is_directory(status)) {
            //std::cout << path << " is a directory" << std::endl;
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                //std::cout << "found file " << entry.path() << std::endl;
                fs::file_status substatus = fs::status(entry.path());
                if (fs::is_regular_file(substatus)) {
                    process_file(entry.path());
                }
            }
        } else if (fs::is_regular_file(status)) {
            //std::cout << path << " is a regular file" << std::endl;
            process_file(path);
        } else {
            std::cerr << path << " is an unsupported file type" << std::endl;
        }
    }
    report();
    return 0;
}
