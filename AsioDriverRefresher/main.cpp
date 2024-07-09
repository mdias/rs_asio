#include "AsioHelpers.h"
#include "AsioSharedHost.h"
#include "stdafx.h"


const AsioHelpers::DriverInfo *findDriverByName(const std::vector<AsioHelpers::DriverInfo> &drivers,
                                                const std::string &name) {
    for (const auto &driver: drivers) {
        if (driver.Name == name) {
            // Return a pointer to the found driver
            return &driver;
        }
    }

    // Return nullptr if no matching driver found
    return nullptr;
}


// Main function simulating DLL behavior
int main(int argc, char *argv[]) {
    // Initialization similar to DllMain DLL_PROCESS_ATTACH
    logging::InitLog();
    logging::info_ts() << " - Wrapper loaded (simulation)" << std::endl;


    std::vector<AsioHelpers::DriverInfo> asioDriversInfo = AsioHelpers::FindDrivers();


    if (argc < 2 || argc > 3) {
        logging::error_ts() << "Usage: <Driver Name> <Optional desired buffer size>" << std::endl;
        return 1;
    }

    unsigned bufferSize = 0;

    if (argc == 3) {
        try {
            bufferSize = std::stoi(argv[2]);
            logging::info_ts() << "Picked up buffer Size: " << bufferSize << std::endl;
        } catch (...) {
            logging::error_ts() << "Usage: <Driver Name> <Optional desired buffer size>" << std::endl;
            return 1;
        }
    }


    const AsioHelpers::DriverInfo *driver = findDriverByName(asioDriversInfo, argv[1]);


    if (driver != nullptr) {
        // Driver with the name found, use *foundDriver to access its members
        logging::info_ts() << "Found driver: " << driver->Name << std::endl;
    } else {
        // Driver with the name not found
        logging::error_ts() << "Driver not found." << std::endl;
        return 1;
    }

    AsioSharedHost *IDriver = CreateAsioHost(*driver);
    IDriver->AddRef();

    if (bufferSize == 0) {
        IDriver->Refresh();
    } else {
        IDriver->Set(bufferSize);
    }

    IDriver->Release();

    logging::info_ts() << "Exiting" << std::endl;
    logging::CleanupLog();

    return 0;
}
