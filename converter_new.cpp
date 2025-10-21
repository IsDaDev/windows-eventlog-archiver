#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

std::string replaceSingleQuotes(const std::string& input) {
    std::string output = input;
    for (size_t i = 0; i < output.size(); ++i) {
        if (output[i] == '\'') {
            output[i] = '"';
        }
    }
    return output;
}

std::string normalizeAttr(std::string input)
{
    input = replaceSingleQuotes(input);

    //  SystemTime="2025-10-20T13:35:38.8040065Z"/  ->  "SystemTime":"2025-10-20T13:35:38.8040065Z"
    int delimiter = input.find('=');
    std::string modified = "\"";

    if (delimiter == std::string::npos) {
        return input;
    }

    modified += input.replace(delimiter, 1, "\":");  // replace '=' with "':"

    if (!modified.empty() && modified.back() == '/') {
        modified = modified.substr(0, modified.size() - 1);
    }

    return modified;
}

void handleSelfClose(std::string input) 
{
    std::vector<std::string> split;
    std::string tmp;

    for (const auto chr : input) {
        if (chr != ' ') {
            tmp += chr;
        } else {
            split.push_back(tmp);
            tmp.clear();
        }
    }

    if (!tmp.empty()) {
        split.push_back(tmp);
    }

    std::cout << "\"" + split[0] + "\": {" << std::endl;

    for (int i = 1; i < split.size(); i++) {
        std::cout << normalizeAttr("_" + split[i]) << std::endl;
        if (i == split.size() - 1) {
            std::cout << "}" << std::endl;
        }
    }
}

void handle(std::string input, std::string sort)
{
    if (sort == "open") {
        std::cout << "Handling Openin" << std::endl;
        std::cout << sort << " " << input << std::endl;
    } else if (sort == "close") {
        std::cout << "Handling Closin" << std::endl;
        std::cout << sort << " " << input << std::endl;
    } else if (sort == "value") {
        std::cout << "Handling Value" << std::endl;
        std::cout << sort << " " << input << std::endl;
    } else if (sort == "sc") {
        std::cout << "Handling Selfclose" << std::endl;
        handleSelfClose(input);
    }
    
}

std::string findKeys(std::string xmlStr)
{
    std::vector<std::string> collection;

    std::string currentKey;
    std::string value;
    std::string lastOpened;
    
    bool start = false;
    for (int i = 0; i < xmlStr.size(); i++) {
        char chr = xmlStr[i];
        
        if (chr == '<') {
            if (!value.empty()) {
                // std::cout << "Value: " + value << std::endl;
                collection.push_back("v: " + value);
            }
            value.clear();
            start = true;
            continue;

        // closing bracket, so key ends
        } else if (chr == '>') {
            if (!currentKey.empty()) {
                // Self-closing tag: <Tag/>
                if (currentKey.back() == '/') {
                    // std::cout << "Self-closing key: " << currentKey << std::endl;
                    collection.push_back("s: " + currentKey);
                    lastOpened.clear();
                }
                // Closing tag: </Tag>
                else if (currentKey[0] == '/') {
                    // std::cout << "Closing key: " << currentKey.substr(1) << std::endl;
                    collection.push_back("c: " + currentKey);
                    lastOpened.clear();
                }
                // Opening tag: <Tag>
                else {
                    lastOpened = currentKey;
                    collection.push_back("o: " + currentKey);
                    // std::cout << "Opening key: " << lastOpened << std::endl;
                }
            }
            
            currentKey.clear();
            start = false;
            continue;
        }

        if (start == true) {
            currentKey += chr;
        }

        if (start == false) {
            value += chr;
        }
    }

    for (const auto line : collection) {
        // std::cout << line << std::endl;
        handle(line.substr(3), line.substr(0,0));
    }

    return "";
}

int main()
{
    std::string read = "<Event xmlns='http://schemas.microsoft.com/win/2004/08/events/event'><System><Provider Name='Microsoft-Windows-Security-Auditing' Guid='{54849625-5478-4994-a5ba-3e3b0328c30d}'/><EventID>5038</EventID><Version>0</Version><Level>0</Level><Task>12290</Task><Opcode>0</Opcode><Keywords>0x8010000000000000</Keywords><TimeCreated SystemTime='2025-10-20T13:36:31.7100797Z'/><EventRecordID>2089320</EventRecordID><Correlation/><Execution ProcessID='4' ThreadID='23548'/><Channel>Security</Channel><Computer>DESKTOP-SV73V84</Computer><Security/></System><EventData><Data Name='param1'>\\Device\\HarddiskVolume3\\Program Files\\Sophos\\Sophos AMSI Protection\\SophosAmsiProvider.dll</Data></EventData></Event>";
    
    findKeys(read);

    for (const char chr : read) {
        // std::cout << chr << std::endl;
        
    }
    
    return 0;
}