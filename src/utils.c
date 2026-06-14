/*
 * Include the utility header file.
 *
 * This header declares the intToStr() function implemented in this file.
 */
#include "../src/hfiles/utils.h"


/*
 * Function name:
 * intToStr
 *
 * Declared in:
 * utils.h
 *
 * Purpose:
 * Converts an integer number into a null-terminated character string.
 *
 * Parameters:
 * num:
 * The integer number that will be converted.
 *
 * str:
 * Pointer to the character array where the resulting string will be stored.
 *
 * Example:
 * intToStr(123, buffer);
 *
 * After the function call:
 * buffer contains "123".
 */
void intToStr(int num, char *str)
{
    /*
     * Create an index variable.
     *
     * This variable keeps track of the current position in the string buffer.
     */
    int i = 0;

    /*
     * Create a flag to remember whether the original number was negative.
     *
     * 0 means the number is not negative.
     * 1 means the number is negative.
     */
    int isNegative = 0;

    /*
     * Check whether the input number is zero.
     *
     * Zero needs to be handled separately because the digit extraction loop
     * below only runs while num is greater than 0.
     */
    if(num == 0)
    {
        /*
         * Store the character '0' in the string.
         *
         * The expression i++ stores the character at str[i] first,
         * then increases i by 1.
         */
        str[i++] = '0';

        /*
         * Add the null terminator at the end of the string.
         *
         * In C, strings must end with '\0'.
         */
        str[i] = '\0';

        /*
         * Return from the function because zero has already been converted.
         */
        return;
    }

    /*
     * Check whether the input number is negative.
     */
    if(num < 0)
    {
        /*
         * Set the negative flag to remember that a minus sign is needed later.
         */
        isNegative = 1;

        /*
         * Convert the number to positive so that digits can be extracted
         * using modulus and division.
         *
         * Example:
         * -123 becomes 123.
         */
        num = -num;
    }

    /*
     * Extract digits from the number one by one.
     *
     * The loop continues until all digits have been processed.
     */
    while(num > 0)
    {
        /*
         * Extract the last digit using num % 10.
         *
         * Example:
         * If num = 123, num % 10 gives 3.
         *
         * Add '0' to convert the digit into its ASCII character.
         * Example:
         * 3 + '0' becomes character '3'.
         *
         * The result is cast to char and stored in the string buffer.
         *
         * Note:
         * Digits are stored in reverse order at this stage.
         */
        str[i++] = (char)((num % 10) + '0');

        /*
         * Remove the last digit from the number.
         *
         * Example:
         * If num = 123, num / 10 becomes 12.
         */
        num /= 10;
    }

    /*
     * If the original number was negative, add the minus sign.
     *
     * At this point, the digits are still stored in reverse order,
     * so the minus sign is also added at the end before reversing.
     */
    if(isNegative)
    {
        /*
         * Add the minus sign character to the string buffer.
         */
        str[i++] = '-';
    }

    /*
     * Add the null terminator to mark the end of the C string.
     */
    str[i] = '\0';

    /*
     * Create the start index for reversing the string.
     *
     * The digits are currently stored in reverse order, so they must be
     * reversed before the string is correct.
     */
    int start = 0;

    /*
     * Create the end index for reversing the string.
     *
     * i is the length of the string, so the last valid character is i - 1.
     */
    int end = i - 1;

    /*
     * Reverse the string in place.
     *
     * This loop swaps the first and last characters, then moves towards
     * the centre of the string.
     */
    while(start < end)
    {
        /*
         * Temporarily store the character at the start position.
         */
        char temp = str[start];

        /*
         * Copy the character from the end position into the start position.
         */
        str[start] = str[end];

        /*
         * Copy the original start character into the end position.
         */
        str[end] = temp;

        /*
         * Move the start index one step forward.
         */
        start++;

        /*
         * Move the end index one step backward.
         */
        end--;
    }
}