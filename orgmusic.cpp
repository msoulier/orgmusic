#define _UNICODE
#include <iostream>
#include <string>
#include <codecvt>
#include <filesystem>
#include <vector>
#include <map>
#include <stdio.h>
#include <cassert>
#include <algorithm>
#include <cctype>

#include <MediaInfo/MediaInfo.h>

#include "config.h"

namespace fs = std::filesystem;

static bool genre = false;
static bool report = false;
static bool dryrun = false;
static std::vector<std::string> input_files;
static std::string outdir;
static std::map<std::string, int> genremap;

bool is_shell_metachar(unsigned char c) {
    // Common shell metacharacters
    const std::string metacharacters = "|&;()<>{}[]$`'\"\\*?~!#^ \t\n";
    return metacharacters.find(c) != std::string::npos;
}

std::string
sane_elem(std::string& insane)
{
    std::string sane = insane;
    // Convert to lower case.
    std::transform(sane.begin(), sane.end(), sane.begin(),
        [](unsigned char c){ return ::tolower(c); });
    // Convert any consecutive spaces to an underscore.
    std::transform(sane.begin(), sane.end(), sane.begin(),
        [](unsigned char c){ return c == ' ' ? '_' : c; });
    // Screen out any unwanted characters.
    sane.erase(
        std::remove_if(sane.begin(), sane.end(),
            [](unsigned char c){ return c > 127; }),
        sane.end());
    sane.erase(
        std::remove_if(sane.begin(), sane.end(), is_shell_metachar),
        sane.end()
    );

    return sane;
}

fs::path outpath(std::string outdir,
                 std::string sgenre,
                 std::string sartist,
                 std::string stitle,
                 fs::path inpath)
{
    std::string full_path = outdir;
    if (genre) {
        full_path += "/" + sane_elem(sgenre);
    }
    full_path += "/" + sane_elem(sartist) + "/" + sane_elem(stitle);
    fs::path path(full_path);
    path.replace_extension(inpath.extension());
    return path;
}

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
        std::cout << details << std::endl;
        std::wstring wgenre = info.Get(MediaInfoLib::Stream_General, 0, L"Genre");
        // convert back
        std::string genre_str = converter.to_bytes(wgenre);
        if (genre_str.empty()) {
            std::cerr << "Unknown Genre: " << spath << std::endl;
        } else {
            if (genre) {
                    //std::cout << genre_str << std::endl;
                    if (genremap.find(genre_str) == genremap.end()) {
                        genremap[genre_str] = 0;
                    }
                    genremap[genre_str] += 1;
            }
        }
        // Album, Album/Artist, Title
        std::wstring walbum = info.Get(MediaInfoLib::Stream_General, 0, L"Album");
        std::string album_str = converter.to_bytes(walbum);
        std::cout << "Album: " << sane_elem(album_str) << std::endl;

        std::wstring wartist = info.Get(MediaInfoLib::Stream_General, 0, L"Performer");
        std::string artist_str = converter.to_bytes(wartist);
        std::cout << "Artist: " << sane_elem(artist_str) << std::endl;

        std::wstring wtitle = info.Get(MediaInfoLib::Stream_General, 0, L"Title");
        std::string title_str = converter.to_bytes(wtitle);
        std::cout << "Title: " << sane_elem(title_str) << std::endl;

        if (!outdir.empty()) {
            std::cout << "Path: " << outpath(outdir, genre_str, artist_str, title_str, path) << std::endl;
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
        } else if ((arg == "--report") || (arg == "-R")) {
            report = true;
            options++;
        } else if ((arg == "--dry-run") || (arg == "-D")) {
            dryrun = true;
            options++;
        } else {
            if (i == argc-1) {
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
        if ((!report) && (outdir.empty())) {
            std::cerr << "output directory is required if not in report mode" << std::endl;
            return -1;
        } else {
            return options;
        }
    }
}

void
generate_report()
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
        std::cerr << "Usage: orgmusic [--genre] [--report] [--dry-run] <input paths> [output path]" << std::endl;
        exit(1);
    }
    assert( argc >= 3 );

    // Test
    //std::string insane = "Shitty Windows File&Name ∞ (too long)";
    //std::cout << sane_elem(insane) << std::endl;
    //exit(0);

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
    if (report) {
        generate_report();
    }
    return 0;
}
