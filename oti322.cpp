#include "oti322.hpp"

OTI322::OTI322() {
    file = open(OTI322_I2C_BUS, O_RDWR);
    if (file < 0) {
        std::cerr << "Error: Unable to open I2C bus " << bus << std::endl;
    } else if (ioctl(file, OTI322_I2C_BUS, OTI322_I2C_ADDR) < 0) {
        std::cerr << "Error: Failed to set I2C address 0x" 
                  << std::hex << I2C_ADDR << std::endl;
        close(file);
        file = -1;
    }
}

OTI322::~OTI322() {
    if (file >= 0) {
        close(file);
    }
}

bool OTI322::readTemperature(float &ambientTemp, float &objectTemp) {
    uint8_t command = 0x80;  // Readout request command

    // Send the readout request command
    if (write(file, &command, 1) != 1) {
        std::cerr << "Error: Failed to send readout command" << std::endl;
        return false;
    }

    // Read 6 bytes of response from sensor
    uint8_t buffer[6] = {0};
    if (read(file, buffer, 6) != 6) {
        std::cerr << "Error: Failed to read temperature data" << std::endl;
        return false;
    }

    // Convert received data to temperature values
    int ambient_raw = (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
    int object_raw = (buffer[5] << 16) | (buffer[4] << 8) | buffer[3];

    // Convert raw data to temperature (Assumption: Scale factor = 0.01)
    ambientTemp = ambient_raw * 0.01;
    objectTemp = object_raw * 0.01;

    return true;
}