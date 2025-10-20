#include <windows.h>
#include <winevt.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "json.hpp"
#pragma comment(lib, "wevtapi.lib")

using json = nlohmann::json;

std::wstring StringToWString(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

std::string WStringToUtf8(const std::wstring &wstr) {
    if (wstr.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0],
                                          (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0],
                        (int)wstr.size(), &result[0], size_needed, NULL, NULL);
    return result;
}

int main(int argc, char ** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " input" << std::endl;
        return -1;
    }

    std::string inputPath = argv[1];

    int dotPos = inputPath.find_last_of(".");

    std::string outputPath = inputPath.substr(0, dotPos) + ".json";

    std::wstring wInput = StringToWString(inputPath);
    EVT_HANDLE hQuery = EvtQuery(
        NULL,
        wInput.c_str(),
        L"*",
        EvtQueryFilePath
    );

    if (!hQuery) {
        std::cerr << "Failed to open EVTX file: " << GetLastError() << std::endl;
        return 1;
    }

    std::ofstream outFile(outputPath);
    if (!outFile) {
        std::cerr << "Cannot open output file.\n";
        return 1;
    }

    EVT_HANDLE events[10];
    DWORD returned = 0;

    while (EvtNext(hQuery, 10, events, INFINITE, 0, &returned)) {
        for (DWORD i = 0; i < returned; i++) {
            DWORD bufferUsed = 0;
            DWORD propertyCount = 0;
            std::vector<wchar_t> xmlBuffer(8192);

            if (!EvtRender(NULL, events[i], EvtRenderEventXml,
                           xmlBuffer.size() * sizeof(wchar_t),
                           &xmlBuffer[0], &bufferUsed, &propertyCount)) {
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                    xmlBuffer.resize(bufferUsed / sizeof(wchar_t));
                    if (!EvtRender(NULL, events[i], EvtRenderEventXml,
                                   xmlBuffer.size() * sizeof(wchar_t),
                                   &xmlBuffer[0], &bufferUsed, &propertyCount)) {
                        std::cerr << "EvtRender failed: " << GetLastError() << std::endl;
                        EvtClose(events[i]);
                        continue;
                    }
                }
            }

            std::wstring xml(xmlBuffer.data(), bufferUsed / sizeof(wchar_t));
            std::string xml_utf8 = WStringToUtf8(xml);

            // Construct simple JSON object
            json j;
            j["xml"] = xml_utf8;

            outFile << j.dump() << "\n";
            EvtClose(events[i]);
        }
    }

    if (GetLastError() != ERROR_NO_MORE_ITEMS) {
        std::cerr << "EvtNext error: " << GetLastError() << std::endl;
    }

    EvtClose(hQuery);
    outFile.close();

    std::cout << "JSON written to " << outputPath << std::endl;
    return 0;
}
