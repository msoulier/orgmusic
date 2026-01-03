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
#include "mstring.hpp"
#include "mlogger.hpp"

namespace fs = std::filesystem;

static std::string version_major = "ORGMUSIC_VERSION_MAJOR";
static std::string version_minor = "ORGMUSIC_VERSION_MINOR";
static std::string version_patch = "ORGMUSIC_VERSION_PATCH";
static bool genre = false;
static bool report = false;
static bool dryrun = false;
static bool dump = false;
static bool verbose = false;
static std::vector<std::string> input_files;
static fs::path outdir;
static std::map<std::string, int> genremap;
static MLogger mlog;

fs::path outpath(fs::path outdir,
                 std::string sgenre,
                 std::string sartist,
                 std::string salbum,
                 std::string stitle,
                 std::string trackno,
                 fs::path inpath)
{
    std::string full_path = outdir.string();
    if (genre) {
        full_path += "/" + sane_elem(sgenre);
    }
    full_path += "/" + sane_elem(sartist) + "/" + sane_elem(salbum) + "/";
    if (trackno != "") {
         full_path += trackno + "_";
    }
    full_path += sane_elem(stitle);
    fs::path path(full_path);
    path.replace_extension(inpath.extension());
    return path;
}

// Return 1 for success, 0 for failure, -1 for fatal error
int
process_file(fs::path path)
{
    // We currently only process .mp3 or .m4a files.
    if ((path.extension() != ".mp3") && (path.extension() != ".m4a")) {
        if (verbose) {
            mlog.error() << "process_file: unsupported file type: " << path.extension() << std::endl;
            mlog.error() << path << std::endl;
        }
        return 0;
    }
    std::string spath = path.string();
    int rv = 0;
    // MediaInfo expects unicode wchar_t*
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring myinput = converter.from_bytes(spath);

    mlog.debug() << "Input file: " << path << std::endl;

    MediaInfoLib::MediaInfo info;
    if (info.Open(myinput) == 0) {
        mlog.error() << "file not opened: " << spath << std::endl;
    } else {
        if (dump) {
            MediaInfoLib::String wdetails = info.Inform();
            std::string details = converter.to_bytes(wdetails);
            std::cout << details << std::endl;
        }
        std::wstring wgenre = info.Get(MediaInfoLib::Stream_General, 0, L"Genre");
        // convert back
        std::string genre_str = converter.to_bytes(wgenre);
        if (genre_str.empty()) {
            mlog.warning() << "Unknown Genre: " << spath << std::endl;
        } else {
            if (genre) {
                    mlog.debug() << genre_str << std::endl;
                    if (genremap.find(genre_str) == genremap.end()) {
                        genremap[genre_str] = 0;
                    }
                    genremap[genre_str] += 1;
            }
        }
        // Album, Album/Artist, Title
        std::wstring walbum = info.Get(MediaInfoLib::Stream_General, 0, L"Album");
        std::string album_str = converter.to_bytes(walbum);
        album_str = album_str.empty() ? "Unknown" : album_str;
        mlog.debug() << "Album: " << sane_elem(album_str) << std::endl;

        std::wstring wartist = info.Get(MediaInfoLib::Stream_General, 0, L"Performer");
        std::string artist_str = converter.to_bytes(wartist);
        artist_str = artist_str.empty() ? "Unknown" : artist_str;
        mlog.debug() << "Artist: " << sane_elem(artist_str) << std::endl;

        std::wstring wtitle = info.Get(MediaInfoLib::Stream_General, 0, L"Title");
        std::string title_str = converter.to_bytes(wtitle);
        title_str = title_str.empty() ? "Unknown" : title_str;
        mlog.debug() << "Title: " << sane_elem(title_str) << std::endl;

        std::wstring wtrackno = info.Get(MediaInfoLib::Stream_General, 0, L"Track name/Position");
        std::string trackno = converter.to_bytes(wtrackno);
        trackno = trackno.empty() ? "" : trackno;
        mlog.debug() << "Track number: " << trackno << std::endl;

        if (!outdir.empty()) {
            fs::path output_file = outpath(outdir, genre_str, artist_str, album_str, title_str, trackno, path);
            mlog.debug() << "Output file: " << output_file << std::endl;
            if (!dryrun) {
                std::cout << "Here I would mkdir: " << output_file << std::endl;
                std::cout << "parent_path: " << output_file.parent_path() << std::endl;
                fs::path parent_path = output_file.parent_path();
                try {
                    if (!fs::exists(parent_path)) {
                        fs::create_directories(parent_path);
                    }
                } catch (fs::filesystem_error err) {
                    mlog.error() << "failed to create directory: " << parent_path << " " << err.what() << std::endl;
                    return -1;
                }
            }
        }

        mlog.debug() << "" << std::endl;

        info.Close();
        rv = 1;
    }
    return rv;
}

int
parse_arguments(int argc, char *argv[])
{
    // FIXME: use a real options parser
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
        } else if ((arg == "--dump") || (arg == "-d")) {
            dump = true;
            options++;
        } else if ((arg == "--verbose") || (arg == "-v")) {
            verbose = true;
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
    if ((report) && (!genre)) {
        std::cerr << "--report requires a report type: --genre currently" << std::endl;
        return -1;
    }
    if (input_files.empty()) {
        return -1;
    } else {
        if ((!report) && (outdir.empty())) {
            std::cerr << "output directory is required if not in report mode" << std::endl;
            return -1;
        }
    }
    if ((!outdir.empty()) && (!fs::exists(outdir))) {
        std::cerr << "ERROR: " << outdir << " does not exist" << std::endl;
        return -1;
    }
    return options;
}

void
generate_report()
{
    std::cout << std::endl;
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
        std::cerr << "Usage: orgmusic [--verbose|-v] [--genre|-g] [--report|-r] [--dry-run|-D] [--dump|-d] <input paths> [output path]" << std::endl;
        std::cerr << "    --dump implies --report" << std::endl;
        std::cerr << "    --report requires a report type (--genre)" << std::endl;
        exit(1);
    }
    assert( argc >= 3 );
    mlog.setDefaults();
    mlog.setLevel(MLoggerVerbosity::info);
    if (verbose) {
        mlog.setLevel(MLoggerVerbosity::debug);
    }
    // Logger now active.

    for (int i = options+1; i < argc; ++i)
    {
        fs::path path(argv[i]);
        fs::file_status status = fs::status(path);
        if (fs::is_directory(status)) {
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                fs::file_status substatus = fs::status(entry.path());
                if (fs::is_regular_file(substatus)) {
                    if (process_file(entry.path()) < 0) {
                        mlog.error() << "failed to process file: " << entry.path() << std::endl;
                        exit(2);
                    }
                }
            }
        } else if (fs::is_regular_file(status)) {
            mlog.debug() << path << " is a regular file" << std::endl;
            if (process_file(path) < 0) {
                mlog.error() << "failed to process file: " << path << std::endl;
                exit(2);
            }
        } else {
            mlog.error() << path << " is an unsupported file type" << std::endl;
        }
    }
    if (report) {
        generate_report();
    }
    return 0;
}
