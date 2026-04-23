#include "besfoc_driver/besfoc_driver.hpp"

besfoc::CanMotor::CanMotor(int can_id, const CanBus& bus, double acc, double dec) : bus_(bus)
{  
    this->tx_can_id_ = can_id + 0x600;
    this->rx_can_id_ = can_id + 0x580; 

    bus_.connect();

    initialize_motor(); // Initialize motors on object creation
    
    // Set default acceleration and deceleration
    cout << "Setting default acceleration and deceleration..." << endl;
    cout << "Acceleration: " << acc << " Deceleration: " << dec << endl;
    set_acceleration(acc);
    set_deceleration(dec);
}

besfoc::CanMotor::~CanMotor() {
    // Destructor allows bus_ to clean up automatically
    // disconnect() will be called by bus_ destructor
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
    
    bus_.send(&frame);
}

void besfoc::CanMotor::initialize_motor() {
    send_can_command({0x60, 0x40}, 0x00, {0x00, 0x06}, WRITE_BYTES_2); // Motor Release
    send_can_command({0x60, 0x40}, 0x00, {0x00, 0x07}, WRITE_BYTES_2); // Motor Ready
    send_can_command({0x60, 0x40}, 0x00, {0x00, 0x0F}, WRITE_BYTES_2); // Motor Switch On
}

void besfoc::CanMotor::set_acceleration(double acceleration) {
    vector<int> data;
    double_to_bytes(acceleration, data);
    send_can_command({0x60, 0x83}, 0x00, data, WRITE_BYTES_4);
}

void besfoc::CanMotor::set_deceleration(double deceleration) {
    vector<int> data;
    double_to_bytes(deceleration, data);
    send_can_command({0x60, 0x84}, 0x00, data, WRITE_BYTES_4);
}

void besfoc::CanMotor::double_to_bytes(double value, vector<int>& bytes) {
    union {
        double doubleValue;
        uint8_t byteArray[8];
    } converter;

    converter.doubleValue = value;

    for (int i = 0; i < 8; i++) {
        bytes.push_back(converter.byteArray[i]);
    }

    cout << "Converted double " << value << " to bytes: ";
    for (int i = 0; i < 8; i++) {
        cout << "Byte " << i << ": " << static_cast<int>(converter.byteArray[i]) << " ";
    }
    cout << endl;
}

void besfoc::CanMotor::stop(){
    bus_.disconnect();
}