#ifndef BESFOC_DRIVER_HPP
#define BESFOC_DRIVER_HPP

#include "linux_canbus_cpp/Bus/CanBus.hpp"
#include <iostream>
#include <vector>
#include <map>

using namespace std;

namespace besfoc
{
    const int RUNNING = 1;
    const int ERROR = 2;
    const int UNINITIALIZED = 4;
    const int IDLE = 8;


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
    class CanMotor
    {
        public:
            CanMotor(int can_id, const CanBus& bus, double acc = 1000.0, double dec = 1000.0);
            ~CanMotor();
            
            void set_velocity(double velocity);
            void set_position(double position);
            void set_acceleration(double acceleration);
            void set_deceleration(double deceleration);

            void read_state(MotorState& state);
            void stop();
        private:
            
            void initialize_motor();
            void double_to_bytes(double value, vector<int>& bytes);

            void send_can_command(array<int, 2> index, int subIndex, vector<int> data, int data_length);
            int tx_can_id_;
            int rx_can_id_;
            CanBus bus_;
            

    };
}
//Define const for status flags

#endif