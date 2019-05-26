#include "common.h"

std::string getPath() {
    std::array<char, 128> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("pwd", "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
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

