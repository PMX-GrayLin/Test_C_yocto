#include "oti322.hpp"

class OTI322 {
public:
    OTI322(const char* bus, int address) {
        this->address = address;
        this->file = open(bus, O_RDWR);
        if (this->file < 0) {
            std::cerr << "Error: Unable to open I2C bus " << bus << std::endl;
        } else if (ioctl(this->file, I2C_SLAVE, address) < 0) {
            std::cerr << "Error: Failed to set I2C address 0x" 
                      << std::hex << address << std::endl;
            close(this->file);
            this->file = -1;
        }
    }

    ~OTI322() {
        if (this->file >= 0) {
            close(this->file);
        }
    }

    bool writeRegister(uint8_t reg, uint8_t value) {
        uint8_t buffer[2] = {reg, value};
        if (write(this->file, buffer, 2) != 2) {
            std::cerr << "Error: Failed to write to I2C device 0x" 
                      << std::hex << address << std::endl;
            return false;
        }
        return true;
    }

    bool readRegister(uint8_t reg, uint8_t &value) {
        if (write(this->file, &reg, 1) != 1) {
            std::cerr << "Error: Failed to select register 0x" 
                      << std::hex << (int)reg << std::endl;
            return false;
        }
        if (read(this->file, &value, 1) != 1) {
            std::cerr << "Error: Failed to read from register 0x" 
                      << std::hex << (int)reg << std::endl;
            return false;
        }
        return true;
    }

private:
    int file;
    int address;
};
