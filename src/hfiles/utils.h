#ifndef UTILS_H
#define UTILS_H

/**
 * Converts an integer number to a null-terminated string.
 *
 * Parameters:
 *
 * num:
 * Integer value to convert.
 *
 * str:
 * Pointer to the character buffer where the converted string will be stored.
 *
 * Example:
 *
 * char buffer[12];
 * intToStr(123, buffer);
 *
 * After this call, buffer contains the string "123".
 *
 * This function is useful in embedded systems where standard functions such
 * as sprintf() may be avoided to reduce memory usage.
 *
 * Note:
 *
 * The buffer must be large enough to store the converted number, optional
 * minus sign, and the null terminator.
 *
 * For a 32-bit integer, a buffer of at least 12 characters is usually safe.
 */
void intToStr(int num, char *str);

#endif