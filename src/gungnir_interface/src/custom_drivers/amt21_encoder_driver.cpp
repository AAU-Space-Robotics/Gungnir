#include "amt21_encoder_driver/amt21_encoder_driver.hpp"
#include <thread>
#include <chrono>
#include <rclcpp/rclcpp.hpp>

using namespace boost::asio;

AMT21::AMT21(const u_int8_t nodeAddress, const std::string& port, unsigned int baud_rate, bool flipTurn)
    : io_context_(),
        serial_(io_context_, port),
        flip(flipTurn)
{
    nodeAddress_ = nodeAddress;
    // Configure serial port
    serial_.set_option(serial_port_base::baud_rate(baud_rate));
    serial_.set_option(serial_port_base::character_size(8));
    serial_.set_option(serial_port_base::parity(serial_port_base::parity::none));
    serial_.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));
    serial_.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));
}

int AMT21::scaleZeroInMiddle(int raw) {
    // Scale the raw value so that zero is in the middle of the range. This is useful for applications like steering where you want to be able to go equally in both directions from a central position.
    int halfRange = MAX_VALUE / 2;
    if(raw > halfRange) {
        return (raw - MAX_VALUE); // Wrap around to negative values
    } else {
        return raw; // Keep as is for values in the lower half
    };
}

int AMT21::readPosition(bool useZeroOffset, bool forceRaw) {
    // Send command
    write(serial_, buffer(&nodeAddress_, 1));

    // Read 2 byte response
    uint8_t response[2];
    boost::system::error_code ec;
    size_t bytesRead = readWithTimeout(response, 2, 500, ec); // 500ms timeout

    if (ec || bytesRead != 2) {
        RCLCPP_ERROR(rclcpp::get_logger("AMT21"), "Read error or timeout at address 0x%02x", nodeAddress_);
        return -1;
    }

    // Combine bytes (low byte first)
    uint16_t raw = (response[1] << 8) | response[0];

    // Validate checksum
    if (!validateChecksum(raw)) {
        RCLCPP_ERROR(rclcpp::get_logger("AMT21"), "Checksum error at address 0x%02x", nodeAddress_);
        return -1;
    }

    // Strip top 2 checksum bits
    uint16_t position = raw & 0x3FFF;

    if(forceRaw) {
        return position; // Return raw position without scaling
    }

    if(useZeroOffset){
        RCLCPP_INFO(rclcpp::get_logger("AMT21"), "Offset Applied");
        position = (position - zeroOffset_ + MAX_VALUE) % MAX_VALUE; // Apply zero offset and wrap around
    }

    if(flip) {
        RCLCPP_INFO(rclcpp::get_logger("AMT21"), "Position before flip: %d", position);
        position = MAX_VALUE - position; // Flip direction if needed
    }

    int scaledPosition = scaleZeroInMiddle(position);

    return scaledPosition; // Convert to radians
}

int AMT21::readMultiTurnPosition(bool useZeroOffset) {

    int singleTurnPos = readPosition(useZeroOffset);

    u_int8_t cmd = nodeAddress_ + 0x01; // Command to read turn counter

    // Send command
    write(serial_, buffer(&cmd, 1));

    // Read 2 byte response
    uint8_t response[2];
    boost::system::error_code ec;
    size_t bytesRead = readWithTimeout(response, 2, 500, ec);

    if(ec || bytesRead != 2) {
        RCLCPP_ERROR(rclcpp::get_logger("AMT21"), "Read error or timeout for turn counter at address 0x%02x", nodeAddress_);
        return -1;
    }

     // Combine bytes (low byte first)
    uint16_t raw = (response[1] << 8) | response[0];

    // Validate checksum
    if (!validateChecksum(raw)) {
        RCLCPP_ERROR(rclcpp::get_logger("AMT21"), "Checksum error at address 0x%02x", nodeAddress_);
        return -1;
    }

    // Strip top 2 checksum bits
    uint16_t turnCount = raw & 0x3FFF;

    RCLCPP_DEBUG(rclcpp::get_logger("AMT21"), "Turn count: %d", turnCount);

    // Calculate multi-turn position
    int multiTurnPosition = turnCount * MAX_VALUE + singleTurnPos;

    return multiTurnPosition;
    // Not implemented yet
}

void AMT21::resetEncoder() {
    uint8_t cmd[2] = { (uint8_t)(nodeAddress_ + 0x02), 0x75 };
    write(serial_, buffer(cmd, 2));
}

void AMT21::setZero() {
    zeroOffset_ = readPosition(false, true);
}

size_t AMT21::readWithTimeout(uint8_t* buf, size_t len, int timeout_ms, boost::system::error_code& ec) {
    size_t bytesRead = 0;
    bool timedOut = false;

    // Set up a timer
    steady_timer timer(io_context_);
    timer.expires_after(std::chrono::milliseconds(timeout_ms));

    // Async read
    async_read(serial_, buffer(buf, len),
        [&](const boost::system::error_code& e, size_t n) {
            ec = e;
            bytesRead = n;
            timer.cancel();
        });

    // Async timer
    timer.async_wait([&](const boost::system::error_code& e) {
        if (!e) {
            timedOut = true;
            serial_.cancel();
        }
    });

    io_context_.run();
    io_context_.restart(); // important - allows reuse

    if (timedOut) {
        ec = make_error_code(boost::system::errc::timed_out);
    }

    return bytesRead;
}

bool AMT21::validateChecksum(uint16_t raw) {
    bool k1 = (raw >> 15) & 1;
    bool k0 = (raw >> 14) & 1;

    uint8_t H = (raw >> 8) & 0x3F;
    uint8_t L = raw & 0xFF;

    bool k1_calc = !(((H>>5)&1) ^ ((H>>3)&1) ^ ((H>>1)&1) ^
                        ((L>>7)&1) ^ ((L>>5)&1) ^ ((L>>3)&1) ^ ((L>>1)&1));

    bool k0_calc = !(((H>>4)&1) ^ ((H>>2)&1) ^ ((H>>0)&1) ^
                        ((L>>6)&1) ^ ((L>>4)&1) ^ ((L>>2)&1) ^ ((L>>0)&1));

    return (k1 == k1_calc) && (k0 == k0_calc);
}