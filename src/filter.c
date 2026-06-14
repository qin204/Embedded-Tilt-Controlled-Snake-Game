/*
 * filter.c  First-order low-pass filter implementation.
 */

#include "../src/hfiles/filter.h"

void LowPassFilter_Init(LowPassFilter *filter, float alpha)
{
    if (filter == 0)
    {
        return;
    }

    filter->value       = 0.0f;
    filter->alpha       = alpha;
    filter->initialized = 0U;
}

void LowPassFilter_Reset(LowPassFilter *filter, float value)
{
    if (filter == 0)
    {
        return;
    }

    filter->value       = value;
    filter->initialized = 1U;
}

float LowPassFilter_Update(LowPassFilter *filter, float new_sample)
{
    if (filter == 0)
    {
        return new_sample;
    }

    if (filter->initialized == 0U)
    {
        filter->value       = new_sample;
        filter->initialized = 1U;
        return filter->value;
    }

    filter->value = (filter->alpha * new_sample) +
                    ((1.0f - filter->alpha) * filter->value);
    return filter->value;
}
