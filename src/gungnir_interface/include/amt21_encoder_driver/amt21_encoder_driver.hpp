#ifndef AMT21_ENCODER_DRIVER_HPP
#define AMT21_ENCODER_DRIVER_HPP

#include <boost/asio.hpp>
#include <iostream>
#include <cstdint>

using namespace boost::asio;

class AMT21 {
public:
    AMT21(const u_int8_t nodeAddress, const std::string& port, unsigned int baud_rate);
    // Read position from encoder at given node address
    // Returns -1 on error
    int readPosition(bool useZeroOffset = true);

    // Reset encoder
    void resetEncoder();

    // Set zero position (single turn encoders only)
    void setZero();

    static const int MAX_VALUE = 0x3FFF;
private:
    

    uint8_t nodeAddress_;
    io_context io_context_;
    serial_port serial_;
    int zeroOffset_ = 0; // For multi-turn encoders, store the zero offset to apply to readings

    // Read with timeout in milliseconds
    size_t readWithTimeout(uint8_t* buf, size_t len, int timeout_ms, boost::system::error_code& ec);

    bool validateChecksum(uint16_t raw);
};

// int main() {
//     try {
//         // Open port at 115200 baud (for E/F/G/H variants)
//         // Use 2000000 for A/B/C/D variants
//         AMT21 encoder("/dev/ttyUSB0", 115200);

//         while (true) {
//             int pos1 = encoder.readPosition(0x54); // encoder 1 default address

//             if (pos1 >= 0)
//                 std::cout << "Encoder 1: " </< std::dec << pos1 << "\n";

//             std::this_thread::sleep_for(std::chrono::milliseconds(10));
//         }
//     }
//     catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << "\n";
//         return 1;
//     }

//     return 0;
// }

#endif // AMT21_ENCODER_DRIVER_HPP