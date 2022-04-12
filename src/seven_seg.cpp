#include <iostream>		// Include all needed libraries here
#include <wiringPi.h>
#include <bits/stdc++.h>

int seven_pins[7]={0,0,0,0,0,0,0};
std::unordered_map<int, std::string> number_mappings;

void set_defaults()
{
    number_mappings.insert(std::make_pair(0, "abcdef"));
    number_mappings.insert(std::make_pair(1, "bc"));
    number_mappings.insert(std::make_pair(2, "abdeg"));
    number_mappings.insert(std::make_pair(3, "abcdg"));
    number_mappings.insert(std::make_pair(4, "bcfg"));
    number_mappings.insert(std::make_pair(5, "acdfg"));
    number_mappings.insert(std::make_pair(6, "acdefg"));
    number_mappings.insert(std::make_pair(7, "abc"));
    number_mappings.insert(std::make_pair(8, "abcdefg"));
    number_mappings.insert(std::make_pair(9, "abcdfg"));

}

void set_number(int number)
{
    std::string pin_list= number_mappings.find(number)->second;
    for(auto &ch:pin_list)
        digitalWrite(seven_pins[(int)(ch-'a')], LOW);
} 

int main()
{
    wiringPiSetup();        // Setup the library
    set_defaults();			// Set up number mappings.
    int switch_pin=0;

    for(int i=0; i<7; i++)  
        pinMode(seven_pins[i], OUTPUT);

    pinMode(switch_pin, INPUT);
    int count=0;
    // Main program loop
    while(1)
    {
            // Button is pressed if digitalRead returns 0
            if(digitalRead(switch_pin) == 1)
                set_number(count++);
    }
        return 0;
}