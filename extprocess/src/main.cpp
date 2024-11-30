#include <chrono>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <thread>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#endif
#include <random>

#include "ver.hpp"

#include <wx/event.h>
#include <wx/string.h>
#include <wx/translation.h>
#include <wx/filename.h>
#include "helpers/sd.hpp"

//#include "../../src/ui/utils.hpp"
#include "libs/SharedMemoryManager.h"
#include "libs/json.hpp"
#include "ui/QueueManager.h"

#include "ApplicationLogic.h"
#include "config.hpp"
#include "helpers/sd.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <dynamiclib_name>" << std::endl;
        return 1;
    }

    std::cout << "[EXTPROCESS] starting with shared memory size: " << SHARED_MEMORY_SIZE << std::endl;

    std::shared_ptr<SharedMemoryManager> sharedMemory = std::make_shared<SharedMemoryManager>(SHARED_MEMORY_PATH, SHARED_MEMORY_SIZE, false);

    if (!sharedMemory) {
        std::cerr << "[EXTPROCESS] Failed to create SharedMemoryManager" << std::endl;
        return 1;
    }

    ApplicationLogic appLogic(argv[1], sharedMemory);

    if (!appLogic.loadLibrary()) {
        std::cerr << "[EXTPROCESS] Can not load shared library" << std::endl;
        writeCriticalLog("[EXTPROCESS] Can not load shared library: " + std::string(argv[1]), "stablediffusiongui_diffuser.log");
        return 1;
    }

    bool needToRun = true;
    int lastId     = 0;
    while (needToRun) {
        
        std::unique_ptr<char[]> buffer(new char[SHARED_MEMORY_SIZE]);


        if (sharedMemory->read(buffer.get(), SHARED_MEMORY_SIZE)) {
            if (std::strlen(buffer.get()) > 0) {
                std::string message = std::string(buffer.get(), SHARED_MEMORY_SIZE);
                if (message.find("exit") != std::string::npos) {
                    std::cout << "Got exit command, exiting... " << std::endl;
                    needToRun = false;
                    break;
                }
                try {
                    nlohmann::json j = nlohmann::json::parse(message);
                    auto item        = j.get<QM::QueueItem>();
                    if (item.id != lastId) {
                        sharedMemory->clear();
                        std::cout << "[EXTPROCESS] New message: " << item.id << std::endl;
                        lastId = item.id;
                        appLogic.processMessage(item);
                    }
                } catch (std::exception& e) {
                    std::cerr << "[EXTPROCESS] Can not parse json message: " << e.what() << std::endl;
                }
            }
        } else {
            std::cerr << "[EXTPROCESS] Can not read shared memory" << std::endl;
            needToRun = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(EPROCESS_SLEEP_TIME));
    }

    return 0;
}
