#ifndef BESFOC_DRIVER_HPP
#define BESFOC_DRIVER_HPP

#include "linux_canbus_cpp/Bus/CanBus.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>

using namespace std;

namespace besfoc
{
    const int RUNNING = 1;
    const int ERROR = 2;
    const int UNINITIALIZED = 4;
    const int IDLE = 8;

    const int POSITION_MODE = 1;
    const int SPEED_MODE = 3;


    const int WRITE_BYTES_1 = 0x2F;
    const int WRITE_BYTES_2 = 0x2B;
    const int WRITE_BYTES_3 = 0x27;
    const int WRITE_BYTES_4 = 0x23;

    map<int, int> data_length_dict = {
        {WRITE_BYTES_1, 1},
        {WRITE_BYTES_2, 2},
        {WRITE_BYTES_3, 3},
        {WRITE_BYTES_4, 4}
    };

    //Define motor state struct
    struct MotorState
    {
        double position;
        double velocity;
        int status;
    };

    //TODO: 1 - Add Reads, 2 - Set New Zero 3 - Class Variables for state

    class CanMotor
    {
        public:
            CanMotor(int can_id, std::shared_ptr<CanBus> bus, int acc = 100, int dec = 100);
            ~CanMotor();

            void set_mode(int mode);
            
            void set_velocity(int velocity);

            void set_zero_position();

            void set_position_relative(int position, int velocity = 100);
            void set_position_absolute(int position, int velocity = 100);

            void set_acceleration(int acceleration);
            void set_deceleration(int deceleration);

            void motor_release();
            void hard_stop();
            void soft_stop();

            void shutdown();

            void read_state(MotorState& state);
            void stop();
        private:
            
            void initialize_motor();
            void to_bytes(int8_t value, vector<int>& bytes);
            void to_bytes(int16_t value, vector<int>& bytes);
            void to_bytes(int32_t value, vector<int>& bytes);

            void send_can_command(array<int, 2> index, int subIndex, vector<int> data, int data_length);
            int tx_can_id_;
            int rx_can_id_;
            std::shared_ptr<CanBus> bus_;
            

    };
}
//Define const for status flags

#endif