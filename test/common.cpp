#include "common.h"

std::string getPath() {
    std::array<char, 128> buffer{0};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("pwd", "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    result.erase(result.find_last_of('\n'), 1);
    return result;
}

int exec(const char *format, ...) {

    char *command = new char[128];
    va_list args;
    va_start(args, format);
    vsprintf(command, format, args);
    va_end(args);
    int ret = system(command);
    delete[] command;
    return ret;
}

