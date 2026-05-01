#include "Logger.h"
#include <fstream>
#include <mutex>
#include <ctime>

std::mutex logMutex;

std::string now() {
    time_t t = time(0);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    return std::string(buf);
}

void log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(logMutex);

    std::ofstream file("Binnacle.log", std::ios::app);
    file << "[" << now() << "] " << msg << std::endl;
}