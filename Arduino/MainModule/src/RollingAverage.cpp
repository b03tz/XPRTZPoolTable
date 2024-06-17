#include "RollingAverage.h"

RollingAverage::RollingAverage(size_t size, uint16_t threshold)
        : size(size), threshold(threshold), index(0), sum(0) {
    values = new uint16_t[size]();
}

RollingAverage::~RollingAverage() {
    delete[] values;
}

void RollingAverage::addValue(uint16_t value) {
    sum -= values[index];
    values[index] = value;
    sum += value;
    index = (index + 1) % size;
}

bool RollingAverage::isThresholdReached() const {
    return (sum / size) <= threshold;
}

uint16_t RollingAverage::getAverage() const {
    return sum / size;
}
