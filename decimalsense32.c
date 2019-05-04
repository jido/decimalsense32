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

char * numberAsString32(decims32 num) {
    static char result[] = "+1.2345678e-99";

    char sign = (num & 0x80000000) ? '-' : '+';
    
    // Getting exponent and mantissa bits
    int16_t expn = (num >> 25) & 0x3f;
    uint32_t mant = num & 0x1ffffff;
    if (expn == 63 && mant == 0x1800000)
    {
        static char inf[] = "+Infinity";
        inf[0] = sign;
        return inf;
    }
    if (expn == 63 && mant > 25000000)
    {
        return "NaN";
    }
    if (expn > 13 && expn < 50)
    {
        uint16_t zexpn = expn - 14;
        uint16_t bucket = zexpn % 3;
        expn = zexpn / 3 - 6;
        if (mant >= 30000000)
        {
            return "NaN";
        }
        mant += (bucket == 0) ? 10000000 :
                (bucket == 1) ? 40000000 :
                                70000000;
    }
    else
    {
        expn = (expn - (expn < 14 ? 16 : 48)) * 3;
        if (mant >= 20000000)
        {
            expn += 2;
            mant -= 20000000;
        }
        else if (mant >= 10000000)
        {
            expn += 1;
            mant -= 10000000;
        }
        if ((expn != -48 && mant < 1000000) || mant >= 10000000)
        {
            return "NaN";
        }
        mant *= 10;
    }
    
    result[0] = sign;
    sprintf(result + 1, "%0.8u", mant);
    memmove(result + 3, result + 2, 7);
    result[2] = '.';
    sprintf(result + 10, "%+d", expn);
    return result;
}

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
        uint64_t large = (exp_a == expn) ? m_a : m_b;
        uint64_t small = (exp_a == expn) ? m_b : m_a;
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

int main(int n, char * args[]) {
    puts(numberAsString32(0x7F7D7840));  // Largest number 5...e+47
    puts(numberAsString32(0xF4240));  // Smallest normal number 1.0e-48
    puts(numberAsString32(0x1C000000));  // Smallest 8-digit precision
    puts(numberAsString32(0x1A000000 + 29999999));  // Largest small 7-digit precision
    puts(numberAsString32(0x00000001));  // Smallest number 1.0e-54
    puts(numberAsString32(makeNumber32(+4, 6985430, +10)));
    //puts(numberAsString32(10000000000000000L));   // Not a number (over 16 digits)
    puts(numberAsString32(0x7f800000));      // +∞
    puts(numberAsString32(makeNumber32(-5, 1658240, +47)));  // -∞
    printf("Zero: 0x%.8x \n", makeNumber32(0, 0, -48));        // 0
    printf("One: 0x%.8x \n", makeNumber32(+1, 0, 0));           // 0x40000000
    printf("Four: 0x%.8x \n", makeNumber32(+4, 0, 0));          // 0x42000000
    printf("Minus five dot five: 0x%.8x \n", makeNumber32(-5, 5000000, 0));    // 0xc2e4e1c0
    puts(numberAsString32(add32(makeNumber32(+1, 0, -48), 123456)));
}

/*
int numberParts_(decimal num, int * expn, uint64_t * decimals)
{
    int e = (num >> 52) & 0x7ff;
    if (e <= 2)
    {
        // Subnormal number
        *expn = 0;
        *decimals = num & 0x003fffffffffffffL;
    }
    else
    {
        // Normal number
        *expn = (e - 1) / 2;
        *decimals = 1000000000000000L + ((num - 0x0010000000000000L) & 0x001fffffffffffffL);
    }
    return num >> 63;   // sign bit
}

uint64_t shiftDecimals(uint64_t decimals, int amount) {
    if (amount < -15 || decimals == 0)
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

decimal add(decimal a, decimal b) {
    int exp_a, exp_b;
    uint64_t m_a, m_b;
    int sign_a = numberParts_(a, &exp_a, &m_a);
    int sign_b = numberParts_(b, &exp_b, &m_b);
    int expn = (exp_a > exp_b) ? exp_a : exp_b;
    uint64_t sign = 0;
    uint64_t sum;
    if (exp_a == exp_b)
    {
        // Same exponent
        if (sign_a != sign_b && ((sign_a == 1 && m_a > m_b) || (sign_b == 1 && m_b > m_a)))
        {
            sign = 1;
        }
        sum = (sign_a == sign_b) ? m_a + m_b : llabs((int64_t) m_a - (int64_t) m_b);
    }
    else
    {
        // Different exponents
        if (sign_a != sign_b && ((sign_a == 1 && exp_a == expn) || (sign_b == 1 && exp_b == expn)))
        {
            sign = 1;
        }
        int exp_diff = abs(exp_b - exp_a);
        uint64_t large = (exp_a == expn) ? m_a : m_b;
        uint64_t small = (exp_a == expn) ? m_b : m_a;
        if (sign_a != sign_b && large < 2000000000000000L)
        {
            // Sum could lose precision
            large *= 10;
            --expn;
            --exp_diff;
        }
        small = shiftDecimals(small, -exp_diff);
        sum = (sign_a == sign_b) ? large + small : llabs((int64_t) large - (int64_t) small);
    }
    if (sum > 9999999999999999L)
    {
        sum /= 10;
        ++expn;
    }
    while (expn != 0 && sum < 1000000000000000L)
    {
        sum *= 10;
        --expn;
    }
    return makeNumber_(sign, sum, expn);
}

decimal opp(decimal num) {
    return num ^ (1L << 63);
}

decimal sub(decimal a, decimal b) {
    return add(a, opp(b));
}

typedef unsigned __int128 uint128_t;

decimal mul(decimal a, decimal b) {
    int exp_a, exp_b;
    uint64_t m_a, m_b;
    int sign_a = numberParts_(a, &exp_a, &m_a);
    int sign_b = numberParts_(b, &exp_b, &m_b);
    uint64_t negat = (sign_a != sign_b);
    int expn = exp_a + exp_b - 511;
    if (expn < 0)
    {
        // Too small, return zero
        return negat << 63;
    }
    else if (expn > 1022)
    {
        // Too large, return infinity
        return 0x7ff0000000000000L | (negat << 63);
    }
    else
    {
        uint64_t prod = ((uint128_t) m_a * m_b) / 1000000000000000L;
        if (prod > 9999999999999999L)
        {
            prod /= 10;
            expn += 1;
        }
        while (expn != 0 && prod < 1000000000000000L)
        {
            prod *= 10;
            --expn;
        }
        return makeNumber_(negat, prod, expn);
    }
}

decimal divs(decimal a, decimal b) {
    int exp_a, exp_b;
    uint64_t m_a, m_b;
    int sign_a = numberParts_(a, &exp_a, &m_a);
    int sign_b = numberParts_(b, &exp_b, &m_b);
    uint64_t negat = (sign_a != sign_b);
    int expn = exp_a - exp_b + 510;
    uint128_t quo = ((uint128_t) m_a * 10000000000000000L) / m_b;
    while (quo > 9999999999999999L) // note: should only loop if b is subnormal
    {
        if (expn == 1022)
        {
            // Too large, return infinity
            return 0x7ff0000000000000L | (negat << 63);
        }
        quo /= 10;
        expn += 1;
    }
    while (quo < 1000000000000000L)
    {
        if (expn == 0) break;       // subnormal number
        quo *= 10;
        --expn;
    }
    return makeNumber_(negat, quo, expn);
}

int main(int n, char * args[]) {
    puts(numberAsString(0x7feFF973CAFA7FFFL));  // Largest number 9.99...e+511
    puts(numberAsString(0x0030000000000000L));  // Smallest normal number 1.0e-510
    puts(numberAsString(0x002386F26FC0FFFFL));  // Largest subnormal number 9.99...e-511
    puts(numberAsString(0x0000000000000001L));  // Smallest number 1.0e-526
    puts(numberAsString(makeNumber(+4, 698543100238333L, +10)));
    puts(numberAsString(10000000000000000L));   // Not a number (over 16 digits)
    puts(numberAsString(0x7ff0000000000000L));      // +∞
    puts(numberAsString(makeNumber(-1, 0, +512)));  // -∞
    printf("Zero: 0x%.16llx \n", makeNumber(0, 0, -511));        // 0
    printf("One: 0x%.16llx \n", makeNumber(+1, 0, 0));           // 0x3ff0000000000000
    printf("Ten: 0x%.16llx \n", makeNumber(+1, 0, +1));          // 0x4010000000000000
    printf("Minus five dot five: 0x%.16llx \n", makeNumber(-5, 500000000000000L, 0));    // 0xbffffcb9e57d4000
    puts(numberAsString(add(makeNumber(+1, 0, -510), 1234567)));
    puts(numberAsString(add(makeNumber(+1, 9, 0), makeNumber(+1, 4, 0))));
    puts(numberAsString(add(makeNumber(-1, 0, -510), 1234567)));
    puts(numberAsString(sub(makeNumber(-1, 0, 0), makeNumber(-1, 234567890000000L, -10))));
    puts(numberAsString(sub(makeNumber(+1, 9, 105), makeNumber(+1, 4, 105))));
    puts(numberAsString(mul(makeNumber(3, 0, +3), makeNumber(5, 0, 0))));
    puts(numberAsString(mul(makeNumber(-1, 0, -510), 1234567)));
    puts(numberAsString(mul(makeNumber(-1, 0, +1), 1234567)));
    puts(numberAsString(divs(makeNumber(3, 0, +3), makeNumber(5, 0, 0))));
    puts(numberAsString(divs(makeNumber(-1, 0, -510), 1234567)));
    puts(numberAsString(divs(makeNumber(-1, 0, +1), 1234567)));
}
*/