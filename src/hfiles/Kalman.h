#ifndef SCALAR_KALMAN_H
#define SCALAR_KALMAN_H

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Structure for a simple scalar Kalman filter.
 *
 * This structure stores all variables required by the Kalman filter.
 * A scalar Kalman filter is used to filter one signal at a time.
 *
 * Examples of signals that can be filtered:
 * - x-axis acceleration
 * - y-axis acceleration
 * - z-axis acceleration
 * - x-axis gyroscope reading
 * - y-axis gyroscope reading
 * - z-axis gyroscope reading
 *
 * Each signal should normally have its own ScalarKalman_t variable.
 *
 * Example:
 * ScalarKalman_t kalman_ax;
 * ScalarKalman_t kalman_ay;
 * ScalarKalman_t kalman_az;
 */
typedef struct
{
    /**
     * Current estimated value.
     *
     * This is the filtered output of the Kalman filter.
     * After each update, x stores the new estimated value.
     */
    float x;

    /**
     * Estimation error covariance.
     *
     * This represents the uncertainty in the current estimated value.
     * A larger value means the filter is less confident in the estimate.
     * A smaller value means the filter is more confident in the estimate.
     */
    float p;

    /**
     * Process or system noise.
     *
     * This value represents how much the actual system value is expected
     * to change between measurements.
     *
     * Larger q:
     * - filter reacts faster to changes
     * - output may be less smooth
     *
     * Smaller q:
     * - output becomes smoother
     * - filter reacts more slowly to changes
     */
    float q;

    /**
     * Measurement noise.
     *
     * This value represents how noisy the sensor measurement is expected
     * to be.
     *
     * Larger r:
     * - filter trusts the sensor measurement less
     * - output becomes smoother
     *
     * Smaller r:
     * - filter trusts the sensor measurement more
     * - output follows the sensor more closely
     */
    float r;

    /**
     * Kalman gain.
     *
     * This value controls how much the filter trusts the new measurement
     * compared with the previous estimate.
     *
     * It is calculated automatically during each update step.
     */
    float k;

    /**
     * Initialisation flag.
     *
     * This variable indicates whether the Kalman filter has already been
     * initialised.
     *
     * Typical meaning:
     * - 0 means the filter has not been initialised.
     * - 1 means the filter has been initialised.
     */
    unsigned char initialized;

} ScalarKalman_t;


/**
 * Initialises the scalar Kalman filter.
 *
 * This function sets the starting value and noise parameters of the filter.
 * It should be called once before using ScalarKalman_Update().
 *
 * Parameters:
 * kf:
 * Pointer to the ScalarKalman_t structure that will be initialised.
 *
 * initial_value:
 * First assumed value of the signal.
 * For example, if the initial acceleration is assumed to be 0,
 * use 0.0f.
 *
 * q:
 * Process or system noise.
 * This controls how quickly the filter can respond to real changes.
 * A common starting value in this project is 0.10f.
 *
 * r:
 * Measurement noise.
 * This controls how much the filter trusts the raw sensor measurement.
 * A common starting value in this project is 0.10f.
 *
 * Return value:
 * None.
 *
 * Example:
 * ScalarKalman_t kalman_ax;
 * ScalarKalman_Init(&kalman_ax, 0.0f, 0.10f, 0.10f);
 */
void ScalarKalman_Init(ScalarKalman_t *kf, float initial_value, float q, float r);


/**
 * Updates the Kalman filter using a new sensor measurement.
 *
 * This function takes a raw measured value and produces a filtered estimate.
 * It should be called each time a new sensor value is available.
 *
 * Parameters:
 * kf:
 * Pointer to the ScalarKalman_t structure used for this signal.
 *
 * measurement:
 * New raw measurement from the sensor.
 *
 * Return value:
 * Filtered or estimated value.
 *
 * Example:
 * float filtered_ax;
 * filtered_ax = ScalarKalman_Update(&kalman_ax, raw_ax);
 *
 * In this example, raw_ax is the raw accelerometer x-axis value,
 * and filtered_ax is the smoothed Kalman output.
 *
 * Note:
 * Each sensor variable should use its own Kalman filter structure.
 * Do not use the same ScalarKalman_t variable for multiple signals.
 */
float ScalarKalman_Update(ScalarKalman_t *kf, float measurement);


/**
 * Resets the Kalman filter estimate to a new value.
 *
 * This function can be used when the system needs to restart the filter
 * without restarting the whole program.
 *
 * Parameters:
 * kf:
 * Pointer to the ScalarKalman_t structure that will be reset.
 *
 * value:
 * New value to store as the current estimate.
 *
 * Return value:
 * None.
 *
 * Example:
 * ScalarKalman_Reset(&kalman_ax, 0.0f);
 *
 * This resets the x-axis acceleration estimate back to 0.
 *
 * Note:
 * This is useful if the sensor is recalibrated, the game restarts,
 * or the filter output needs to be cleared.
 */
void ScalarKalman_Reset(ScalarKalman_t *kf, float value);


#ifdef __cplusplus
}
#endif

#endif