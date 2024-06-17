#ifndef ROLLING_AVERAGE_H
#define ROLLING_AVERAGE_H

#include <Arduino.h>

class RollingAverage {
public:
    RollingAverage(size_t size, uint16_t threshold);
    ~RollingAverage();

    void addValue(uint16_t value);
    bool isThresholdReached() const;
    uint16_t getAverage() const;

private:
    size_t size;
    uint16_t threshold;
    size_t index;
    uint16_t* values;
    uint32_t sum;
};

#endif // ROLLING_AVERAGE_H
