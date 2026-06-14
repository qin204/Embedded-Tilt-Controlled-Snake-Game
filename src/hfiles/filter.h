#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>

/*
 * First-order low-pass filter for a single scalar signal.
 *
 * Specification formula:
 *   filtered = alpha * new_sample + (1 - alpha) * previous_filtered
 */
typedef struct
{
    float    value;
    float    alpha;
    uint8_t  initialized;
} LowPassFilter;

/*
 * Initialises a low-pass filter.
 *
 * alpha: smoothing factor in (0, 1].  Smaller values produce smoother output.
 */
void LowPassFilter_Init(LowPassFilter *filter, float alpha);

/*
 * Resets the filter to a new starting value.
 */
void LowPassFilter_Reset(LowPassFilter *filter, float value);

/*
 * Updates the filter with a new sample and returns the filtered result.
 */
float LowPassFilter_Update(LowPassFilter *filter, float new_sample);

#endif /* FILTER_H */
