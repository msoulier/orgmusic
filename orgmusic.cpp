#define _UNICODE
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>
#include <MediaInfo/MediaInfo.h>

#include "config.h"

int
main(int argc, char *argv[])
{
    if (argc > 1) {
        // MediaInfo expects unicode wchar_t*
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring myinput = converter.from_bytes(argv[1]);

        MediaInfoLib::MediaInfo info;
        if (info.Open(myinput) == 0) {
            std::cerr << "ERROR: file not opened" << std::endl;
            return 1;
        } else {
            std::cout << "file opened" << std::endl;
        }
        MediaInfoLib::String wdetails = info.Inform();
        std::string details = converter.to_bytes(wdetails);
        std::cout << details << std::endl;
        std::wstring genre = info.Get(MediaInfoLib::Stream_General, 0, L"Genre");
        // convert back
        std::string genre_str = converter.to_bytes(genre);
        if (genre_str.empty()) {
            std::cerr << "Unknown Genre" << std::endl;
        } else {
            std::cout << genre_str << std::endl;
        }
        info.Close();
    }
    return 0;
}
