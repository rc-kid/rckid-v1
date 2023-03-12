#include <cstdio>
#include <iostream>
#include <vector>
#include <algorithm>

constexpr unsigned REG_VALUES[] = {
    // E12
    10,	12,	15,	18,	22,	27, 33,	39,	47,	56,	68,	82,
    100, 120, 150, 180, 220, 270, 330, 390, 470, 560, 680, 820,
    // E96
    //100, 102, 105, 107,	110, 113,
    //115, 118, 121, 124, 127, 130,
    //133, 137, 140, 143, 147, 150,
    //154, 158, 162, 165, 169, 174,
    //178, 182, 187, 191, 196, 200,
    //205, 210, 215, 221, 226, 232,
    //237, 243, 249, 255, 261, 267,
    //274, 280, 287, 294, 301, 309,
    //316, 324, 332, 340, 348, 357,
    //365, 374, 383, 392, 402, 412,
    //422, 432, 442, 453, 464, 475,
    //487, 499, 511, 523, 536, 549,
    //562, 576, 590, 604, 619, 634,
    //649, 665, 681, 698, 715, 732,
    //750, 768, 787, 806, 825, 845,
    //866, 887, 909, 931, 953, 976,
};

struct Solution {
    unsigned rgnd;
    unsigned r1;
    unsigned r2;
    unsigned r3;



    double simulateValue(bool r1, bool r2, bool r3) {
        double x = 0;
        if (r1) 
            x += 1.0 / this->r1;
        if (r2) 
            x += 1.0 / this->r2;
        if (r3) 
            x += 1.0 / this->r3;
        if (x == 0) 
            return 1;
        x = 1.0 / x;
        x = x / ( x + rgnd); // buttons close to ground
        return x;
    }

    unsigned simulateSolution(double vcc = 0) {
        std::vector<double> values;
        for (unsigned x = 0; x < 8; ++x) 
            values.push_back(simulateValue(x & 1, x & 2, x & 4));
        if (vcc != 0) {
            for (unsigned i = 0; i < values.size(); ++i)
            std::cout << "    " << (i & 1) << (i & 2) << (i & 4) << " : " << values[i] * vcc << std::endl;
        }
        std::sort(values.begin(), values.end());
        double diff = 1;
        for (size_t i = 1; i < values.size(); ++i) {
            if (diff > values[i] - values[i - 1])
                diff = values[i] - values[i - 1];
        }
        return diff * 1023;
    }

};

//Best solution (diff : 41)
//  RGND: 82, RA: 82, RB: 150, RC: 270
//    000 : 255
//    100 : 127.5
//    020 : 164.871
//    120 : 100.131
//    004 : 195.597
//    104 : 110.691
//    024 : 137.81
//    124 : 89.4621

int main(int argc, char * argv[]) {
    unsigned minDiff = 0;
    for (unsigned rgnd : REG_VALUES) {
        for (unsigned ra : REG_VALUES) {
            for (unsigned rb : REG_VALUES){
                for (unsigned rc : REG_VALUES) {
                    Solution s{ rgnd, ra, rb, rc};
                    uint8_t d = s.simulateSolution();
                    if (d > minDiff) {
                        minDiff = d;
                        std::cout << "Best solution (diff : " << (int)d << ")" << std::endl;
                        std::cout << "  RGND: " << rgnd << ", RA: " << ra << ", RB: " << rb << ", RC: " << rc <<  std::endl;
                        s.simulateSolution(255);
                    }
                }
            }
        }
    }
    return EXIT_SUCCESS;
}