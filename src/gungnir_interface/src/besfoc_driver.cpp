#include "besfoc_driver/besfoc_driver.hpp"

besfoc::CanMotor::CanMotor(int can_id, std::shared_ptr<CanBus> bus, int acc, int dec)
    : tx_can_id_(can_id + 0x600), rx_can_id_(can_id + 0x580), bus_(bus)
{  

    initialize_motor(); // Initialize motors on object creation
    

    // Set default acceleration and deceleration
    cout << "Setting default acceleration and deceleration..." << endl;
    cout << "Acceleration: " << acc << " Deceleration: " << dec << endl;
    set_acceleration(acc);
    set_deceleration(dec);
}

void besfoc::CanMotor::send_can_command(array<int, 2> index, int subIndex, vector<int> data, int data_length)
{
    // Construct CAN frame based on command parameters
    CanFrame frame;
    frame.frame.can_id = this->tx_can_id_;
    frame.frame.can_dlc = 8;

    frame.data()[0] = data_length;
    frame.data()[1] = index[1];
    frame.data()[2] = index[0];
    frame.data()[3] = subIndex;

    int data_length_bytes = data_length_dict[data_length];

    for(int i = data_length_bytes; i > 0; i--) {
        cout << "Data byte " << i << ": " << data[i - 1] << endl;
        frame.data()[3 + (data_length_bytes - i + 1)] = data[i - 1];
    }
    // Send the CAN frame
    
    bus_->send(&frame);
}

void besfoc::CanMotor::initialize_motor() {
    send_can_command({0x60, 0x40}, 0x00, {0x00, 0x06}, WRITE_BYTES_2); // Motor Release
    send_can_command({0x60, 0x40}, 0x00, {0x00, 0x07}, WRITE_BYTES_2); // Motor Ready
    send_can_command({0x60, 0x40}, 0x00, {0x00, 0x0F}, WRITE_BYTES_2); // Motor Switch On
}

void besfoc::CanMotor::set_mode(int mode) {
    send_can_command({0x60, 0x60}, 0x00, {static_cast<int>(mode)}, WRITE_BYTES_1);
}

void besfoc::CanMotor::set_velocity(int velocity) {

    if(abs(velocity) > 6000) {
        velocity = (velocity > 0) ? 6000 : -6000; // Limit velocity to valid range
    }

    vector<int> data;
    to_bytes(velocity, data);
    send_can_command({0x60, 0xFF}, 0x00, data, WRITE_BYTES_4);
}

void besfoc::CanMotor::set_position_relative(int position, int velocity) {
    vector<int> posData;
    vector<int> velData;

    to_bytes(position, posData);

    if(abs(position) > 100000000) {
        return; // Invalid position value, do not send command
    }

    if (velocity > 0 && velocity <= 6000) {
        to_bytes(velocity, velData);
    } else {
        to_bytes(100, velData); // Default velocity if invalid value provided
    }

    send_can_command({0x60, 0x81}, 0x00, velData, WRITE_BYTES_4); // Set speed
    send_can_command({0x60, 0x7A}, 0x00, posData, WRITE_BYTES_4); // Set position

    //Start Relative Positioning Movement
    send_can_command({0x60, 0x40}, 0x00, {0x00, 0x0F}, WRITE_BYTES_2);
    send_can_command({0x60, 0x40}, 0x00, {0x00, 0x4F}, WRITE_BYTES_2); 
    send_can_command({0x60, 0x40}, 0x00, {0x00, 0x5F}, WRITE_BYTES_2); 
}

void besfoc::CanMotor::set_position_absolute(int position, int velocity) {
    vector<int> posData;
    vector<int> velData;

    to_bytes(position, posData);

    if(abs(position) > 100000000) {
        return; // Invalid position value, do not send command
    }

    if (velocity > 0 && velocity <= 6000) {
        to_bytes(velocity, velData);
    } else {
        to_bytes(100, velData); // Default velocity if invalid value provided
    }

    send_can_command({0x60, 0x81}, 0x00, velData, WRITE_BYTES_4); // Set speed
    send_can_command({0x60, 0xA4}, 0x00, posData, WRITE_BYTES_4); // Set position

    //Start Absolute Positioning Movement
    send_can_command({0x60, 0x40}, 0x00, {0x00, 0x0F}, WRITE_BYTES_2);
    send_can_command({0x60, 0x40}, 0x00, {0x00, 0x1F}, WRITE_BYTES_2); 
}

void besfoc::CanMotor::set_acceleration(int acceleration) {
    vector<int> data;
    to_bytes(acceleration, data);
    send_can_command({0x60, 0x83}, 0x00, data, WRITE_BYTES_4);
}

void besfoc::CanMotor::set_deceleration(int deceleration) {
    vector<int> data;
    to_bytes(deceleration, data);
    send_can_command({0x60, 0x84}, 0x00, data, WRITE_BYTES_4);
}

void besfoc::CanMotor::soft_stop() {
    send_can_command({0x60, 0x5A}, 0x00, {0x00, 0x05}, WRITE_BYTES_2);
}

void besfoc::CanMotor::hard_stop() {
    send_can_command({0x60, 0x5A}, 0x00, {0x00, 0x06}, WRITE_BYTES_2);
}

void besfoc::CanMotor::motor_release() {
    send_can_command({0x60, 0x5A}, 0x00, {0x00, 0x00}, WRITE_BYTES_2);
}

void besfoc::CanMotor::to_bytes(int32_t value, vector<int>& bytes) {
    for(int i = 0; i < 4; i++) {
        bytes.push_back((value >> (8 * i)) & 0xFF);
    }
    reverse(bytes.begin(), bytes.end()); // Reverse to get big-endian format
}

void besfoc::CanMotor::to_bytes(int16_t value, vector<int>& bytes) {
    for(int i = 0; i < 2; i++) {
        bytes.push_back((value >> (8 * i)) & 0xFF);
    }
    reverse(bytes.begin(), bytes.end()); // Reverse to get big-endian format
}

void besfoc::CanMotor::to_bytes(int8_t value, vector<int>& bytes) {
    bytes.push_back(value & 0xFF);
}


besfoc::CanMotor::~CanMotor() {
    if (bus_) {
        bus_->disconnect();
    }
}