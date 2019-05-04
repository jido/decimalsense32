/*
 Decimal 32 bit numbers that make sense

 Format:
 =======
 seeeeeem mmmmmmmm mmmmmmmm mmmmmmmm

 s = sign bit
 e = 6 exponent bits
 m = 25 mantissa bits

 Normal numbers
 ==============

 * exponent range 0-13

  M = decimal representation of mantissa bits
  Exponent = (exponent bits - 16) * 3 + M[0]
  Mantissa = M[1:7]

 * exponent range 14-49

  Exponent = (exponent bits - 14) / 3 - 6
  Bucket = (exponent bits - 14) % 3
  Mantissa = mantissa bits +
    10,000,000 for bucket 0
    40,000,000 for bucket 1
    70,000,000 for bucket 2

 * exponent range 50-63

  M = decimal representation of mantissa bits
  Exponent = (exponent bits - 48) * 3 + M[0]
  Mantissa = M[1:7]

 Subnormal numbers
 =================

 Simple extension of normal numbers with exponent bits = 0 and mantissa bits = 0 to 999,999
 Smallest non-zero: 1.0e-54
 Largest: 0.999,999e-48
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef uint32_t decims32;

decims32 makeNumber32_(int sign, uint32_t decimals, int expn) {
    uint32_t units;
    if (expn >= -6 && expn < 6)
    {
        units = decimals / 10000000;
        decimals = decimals % 10000000;
        // Place 8-digit value in the appropriate bucket
        if (units >= 7)
        {
            units -= 7;
            expn = (expn + 6) * 3 + 16;
        }
        else if (units >= 4)
        {
            units -= 4;
            expn = (expn + 6) * 3 + 15;
        }
        else
        {
            units -= 1;
            expn = (expn + 6) * 3 + 14;
        }
    }
    else
    {
        decimals = decimals / 10;               // reduce precision
        expn += (expn < -6 ? 48 : 50 * 3 - 6);
        units = expn % 3;                       // use units place for exponent offset
        expn = expn / 3;
    }
    return (sign << 31) | (expn << 25) | (units * 10000000 + decimals);
}

decims32 makeNumber32(int units, uint32_t decimals, int expn) {
    uint32_t sign = (units < 0);
    units = abs(units);
    if (expn >= -6 && expn < 6)
    {
        // Place 8-digit value in the appropriate bucket
        if (units >= 7)
        {
            units -= 7;
            expn = (expn + 6) * 3 + 16;
        }
        else if (units >= 4)
        {
            units -= 4;
            expn = (expn + 6) * 3 + 15;
        }
        else
        {
            units -= 1;
            expn = (expn + 6) * 3 + 14;
        }
    }
    else
    {
        decimals = units * 1000000 + decimals / 10;     // reduce precision
        expn += (expn < -6 ? 48 : 50 * 3 - 6);
        units = expn % 3;                               // use units place for exponent offset
        expn = expn / 3;
    }
    return (sign << 31) | (expn << 25) | (decimals + units * 10000000);
}

int numberParts32_(decims32 num, int * expn, uint32_t * decimals)
{
    // Getting exponent and mantissa bits
    int16_t exponent = (num >> 25) & 0x3f;
    uint32_t mantissa = num & 0x1ffffff;
    if (exponent > 13 && exponent < 50)
    {
        uint16_t zexpn = exponent - 14;
        uint16_t bucket = zexpn % 3;
        *expn = zexpn / 3 - 6;
        *decimals = mantissa + (
            (bucket == 0) ? 10000000 :
            (bucket == 1) ? 40000000 :
                            70000000);
    }
    else
    {
        *expn = (exponent - (exponent < 14 ? 16 : 48)) * 3;
        if (mantissa >= 20000000)
        {
            *expn += 2;
            mantissa -= 20000000;
        }
        else if (mantissa >= 10000000)
        {
            *expn += 1;
            mantissa -= 10000000;
        }
        *decimals = mantissa * 10;
    }
    return num >> 31;
}

char * numberAsString32(decims32 num) {
    static char result[] = "+1.2345678e-99";

    // Getting exponent and mantissa bits
    int expn;
    uint32_t mant;
    char sign = (numberParts32_(num, &expn, &mant) ? '-' : '+');
    
    if (num == 0xFF800000 || num == 0x7F800000)
    {
        static char inf[] = "+Infinity";
        inf[0] = sign;
        return inf;
    }
    if (expn == 47 && mant > 50000000)
    {
        return "NaN";
    }
    uint32_t mantbits = num & 0x1ffffff;
    if (mantbits >= 30000000)
    {
        return "NaN";
    }
    if ((expn != -48 && expn < -6) || expn >= 6)
    {
        if (mantbits % 10000000 < 1000000)
        {
            return "NaN";
        }
    }
    
    result[0] = sign;
    sprintf(result + 1, "%0.8u", mant);
    memmove(result + 3, result + 2, 7);
    result[2] = '.';
    sprintf(result + 10, "%+d", expn);
    return result;
}

uint32_t shiftDecimals32(uint32_t decimals, int amount) {
    if (amount < -8 || decimals == 0)
    {
        return 0;
    }
    else if (amount < 0)
    {
        switch ((-amount) % 4)
        {
            case 1:
            decimals /= 10;
            break;
            
            case 2:
            decimals /= 100;
            break;
            
            case 3:
            decimals /= 1000;
            break;
        }
        amount = amount / 4 * 4;
        for (int t = 0; t > amount && decimals != 0; t -= 4)
        {
            decimals /= 10000;
        }
        return decimals;
    }
    else
    {
        uint32_t fac = 1;
        for (int h = 0; h < amount; ++h)
        {
            fac *= 10;
        }
        return decimals * fac;
    }
}

decims32 add32(decims32 a, decims32 b) {
    int exp_a, exp_b;
    uint32_t m_a, m_b;
    int sign_a = numberParts32_(a, &exp_a, &m_a);
    int sign_b = numberParts32_(b, &exp_b, &m_b);
    int expn = (exp_a > exp_b) ? exp_a : exp_b;
    uint32_t sign = 0;
    uint32_t sum;
    if (exp_a == exp_b)
    {
        // Same exponent
        if (sign_a != sign_b && ((sign_a == 1 && m_a > m_b) || (sign_b == 1 && m_b > m_a)))
        {
            sign = 1;
        }
        sum = (sign_a == sign_b) ? m_a + m_b : abs((int32_t) m_a - (int32_t) m_b);
    }
    else
    {
        // Different exponents
        if (sign_a != sign_b && ((sign_a == 1 && exp_a == expn) || (sign_b == 1 && exp_b == expn)))
        {
            sign = 1;
        }
        int exp_diff = abs(exp_b - exp_a);
        uint32_t large = (exp_a == expn) ? m_a : m_b;
        uint32_t small = (exp_a == expn) ? m_b : m_a;
        if (sign_a != sign_b)
        {
            // Sum could lose precision
            large *= 10;
            --expn;
            --exp_diff;
        }
        small = shiftDecimals32(small, -exp_diff);
        sum = (sign_a == sign_b) ? large + small : abs((int32_t) large - (int32_t) small);
    }
    if (sum > 99999999)
    {
        sum /= 10;
        ++expn;
    }
    while (expn != -48 && sum < 10000000)
    {
        sum *= 10;
        --expn;
    }
    return makeNumber32_(sign, sum, expn);
}

decims32 opp32(decims32 num) {
    return num ^ (1 << 31);
}

decims32 sub32(decims32 a, decims32 b) {
    return add32(a, opp32(b));
}

decims32 mul32(decims32 a, decims32 b) {
    int exp_a, exp_b;
    uint32_t m_a, m_b;
    int sign_a = numberParts32_(a, &exp_a, &m_a);
    int sign_b = numberParts32_(b, &exp_b, &m_b);
    uint32_t negat = (sign_a != sign_b);
    int expn = exp_a + exp_b;
    if (expn < -48)
    {
        // Too small, return zero
        return negat << 31;
    }
    else if (expn > 47)
    {
        // Too large, return infinity
        return 0x7f800000 | (negat << 31);
    }
    else
    {
        uint32_t prod = ((uint64_t) m_a * m_b) / 10000000;
        if (prod > 99999999)
        {
            prod /= 10;
            expn += 1;
        }
        while (expn != 0 && prod < 10000000)
        {
            prod *= 10;
            --expn;
        }
        return makeNumber32_(negat, prod, expn);
    }
}

decims32 div32(decims32 a, decims32 b) {
    int exp_a, exp_b;
    uint32_t m_a, m_b;
    int sign_a = numberParts32_(a, &exp_a, &m_a);
    int sign_b = numberParts32_(b, &exp_b, &m_b);
    int negat = (sign_a != sign_b);
    int expn = exp_a - exp_b;
    uint64_t quo = ((uint64_t) m_a * 10000000) / m_b;
    while (quo > 99999999 || (quo > 50000000 && expn == 47)) // note: should only loop if b is subnormal
    {
        if (expn == 47)
        {
            // Too large, return infinity
            return 0x7f800000 | (negat << 31);
        }
        quo /= 10;
        expn += 1;
    }
    while (quo < 10000000)
    {
        if (expn == 0) break;       // subnormal number
        quo *= 10;
        --expn;
    }
    return makeNumber32_(negat, quo, expn);
}

int main(int n, char * args[]) {
    printf("Largest number: %s\n", numberAsString32(0x7F7D7840));  // 5.0e+47
    printf("Smallest normal: %s\n", numberAsString32(0xF4240));    // 1.0e-48
    printf("Eight significant digits: %s\n", numberAsString32(0x1C000000));  // Smallest 8-digit precision 1.0e-6
    printf("Seven significant digits: %s\n", numberAsString32(0x1A000000 + 29999999));  // Largest small 7-digit precision 9.999,999e-7
    printf("Smallest number (1.0e-54): %s\n", numberAsString32(0x00000001));
    printf("Number 4.698,543e10: %s\n", numberAsString32(makeNumber32(+4, 6985430, +10)));
    printf("Invalid raw value: %s\n", numberAsString32(20000000));    // Not a number (missing number in units place)
    printf(" +∞: %s\n", numberAsString32(0x7f800000));
    printf(" -∞: %s\n", numberAsString32(makeNumber32(-5, 1658240, +47)));
    printf("Zero: 0x%.8x \n", makeNumber32(0, 0, -48));        // 0
    printf("One: 0x%.8x \n", makeNumber32(+1, 0, 0));           // 0x40000000
    printf("Four: 0x%.8x \n", makeNumber32(+4, 0, 0));          // 0x42000000
    printf("Minus five dot five: 0x%.8x \n", makeNumber32(-5, 5000000, 0));    // 0xc2e4e1c0
    printf(" 1.0e-47 + 0.123456e-48 = %s\n", numberAsString32(add32(makeNumber32(+1, 0, -47), 123456)));
    printf(" 1.0000009 + 1.0000004 = %s\n", numberAsString32(add32(makeNumber32(+1, 9, 0), makeNumber32(+1, 4, 0))));
    printf(" -1.0e-48 + 0.123456e-48 = %s\n", numberAsString32(add32(makeNumber32(-1, 0, -48), 123456)));
    printf(" -1.0 - -1.2345678e-6 = %s\n", numberAsString32(sub32(makeNumber32(-1, 0, 0), makeNumber32(-1, 2345678, -6))));
    printf(" 1.0000009e4 - 1.0000004e4 = %s\n", numberAsString32(sub32(makeNumber32(+1, 9, +4), makeNumber32(+1, 4, +4))));
    printf(" 3.0e3 * 5.0 = %s\n", numberAsString32(mul32(makeNumber32(3, 0, +3), makeNumber32(5, 0, 0))));
    printf(" -1.0e-48 * 0.123456e-48 = %s\n", numberAsString32(mul32(makeNumber32(-1, 0, -48), 123456)));
    printf(" 10.0 * 0.123456e-48 = %s\n", numberAsString32(mul32(makeNumber32(-1, 0, +1), 123456)));
    printf(" 3.0e3 / 5.0 = %s\n", numberAsString32(div32(makeNumber32(3, 0, +3), makeNumber32(5, 0, 0))));
    printf(" -1.0e-48 / 0.123456e-48 = %s\n", numberAsString32(div32(makeNumber32(-1, 0, -48), 123456)));
    printf(" -3.0e-2 / 0.06e-48 = %s\n", numberAsString32(div32(makeNumber32(-3, 0, -2), 60000)));
}
