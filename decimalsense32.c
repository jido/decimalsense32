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

//__attribute__((always_inline))
decims32 makeNumber32_(int sign, uint32_t decimals, int expn) {
    if (expn >= -6 && expn < 6)
    {
        // Place 8-digit value in the appropriate bucket
        if (decimals >= 70000000)
        {
            decimals -= 70000000;
            expn = (expn + 6) * 3 + 16;
        }
        else if (decimals >= 40000000)
        {
            decimals -= 40000000;
            expn = (expn + 6) * 3 + 15;
        }
        else
        {
            decimals -= 10000000;
            expn = (expn + 6) * 3 + 14;
        }
    }
    else
    {
        decimals = decimals / 10;               // reduce precision
        expn += (expn < -6 ? 48 : 50 * 3 - 6);
        uint32_t units = expn % 3;              // use units place for exponent offset
        expn = expn / 3;
        decimals += units * 10000000;
    }
    return (sign << 31) | (expn << 25) | decimals;
}

decims32 asDecimal32(double f) {
    
    static uint64_t powersOf2[340] = { // First nine non-zero digits of powers of 2 in range (-180 to 160) - 52 
        144890865, 289781731, 579563461, 115912692, 231825384, 463650769, 927301538, 185460308, 370920615, 741841230, 148368246, 296736492, 593472984, 118694597, 237389194, 
        474778387, 949556775, 189911355, 379822710, 759645420, 151929084, 303858168, 607716336, 121543267, 243086534, 486173069, 972346137, 194469227, 388938455, 777876910, 
        155575382, 311150764, 622301528, 124460306, 248920611, 497841222, 995682444, 199136489, 398272978, 796545956, 159309191, 318618382, 637236764, 127447353, 254894706, 
        509789412, 101957882, 203915765, 407831529, 815663058, 163132612, 326265223, 652530447, 130506089, 261012179, 522024357, 104404871, 208809743, 417619486, 835238972, 
        167047794, 334095589, 668191178, 133638236, 267276471, 534552942, 106910588, 213821177, 427642354, 855284707, 171056941, 342113883, 684227766, 136845553, 273691106, 
        547382213, 109476443, 218952885, 437905770, 875811540, 175162308, 350324616, 700649232, 140129846, 280259693, 560519386, 112103877, 224207754, 448415509, 896831017, 
        179366203, 358732407, 717464814, 143492963, 286985925, 573971851, 114794370, 229588740, 459177481, 918354962, 183670992, 367341985, 734683969, 146936794, 293873588, 
        587747175, 117549435, 235098870, 470197740, 940395481, 188079096, 376158192, 752316385, 150463277, 300926554, 601853108, 120370622, 240741243, 481482486, 962964972, 
        192592994, 385185989, 770371978, 154074396, 308148791, 616297582, 123259516, 246519033, 493038066, 986076132, 197215226, 394430453, 788860905, 157772181, 315544362, 
        631088724, 126217745, 252435490, 504870979, 100974196, 201948392, 403896783, 807793567, 161558713, 323117427, 646234854, 129246971, 258493941, 516987883, 103397577, 
        206795153, 413590306, 827180613, 165436123, 330872245, 661744490, 132348898, 264697796, 529395592, 105879118, 211758237, 423516474, 847032947, 169406589, 338813179, 
        677626358, 135525272, 271050543, 542101086, 108420217, 216840434, 433680869, 867361738, 173472348, 346944695, 693889390, 138777878, 277555756, 555111512, 111022302, 
        222044605, 444089210, 888178420, 177635684, 355271368, 710542736, 142108547, 284217094, 568434189, 113686838, 227373675, 454747351, 909494702, 181898940, 363797881, 
        727595761, 145519152, 291038305, 582076609, 116415322, 232830644, 465661287, 931322575, 186264515, 372529030, 745058060, 149011612, 298023224, 596046448, 119209290, 
        238418579, 476837158, 953674316, 190734863, 381469727, 762939453, 152587891, 305175781, 610351563, 122070313, 244140625, 488281250, 976562500, 195312500, 390625000, 
        781250000, 156250000, 312500000, 625000000, 125000000, 250000000, 500000000, 100000000, 200000000, 400000000, 800000000, 160000000, 320000000, 640000000, 128000000, 
        256000000, 512000000, 102400000, 204800000, 409600000, 819200000, 163840000, 327680000, 655360000, 131072000, 262144000, 524288000, 104857600, 209715200, 419430400, 
        838860800, 167772160, 335544320, 671088640, 134217728, 268435456, 536870912, 107374182, 214748365, 429496730, 858993459, 171798692, 343597384, 687194767, 137438953, 
        274877907, 549755814, 109951163, 219902326, 439804651, 879609302, 175921860, 351843721, 703687442, 140737488, 281474977, 562949953, 112589991, 225179981, 450359963, 
        900719925, 180143985, 360287970, 720575940, 144115188, 288230376, 576460752, 115292150, 230584301, 461168602, 922337204, 184467441, 368934881, 737869763, 147573953, 
        295147905, 590295810, 118059162, 236118324, 472236648, 944473297, 188894659, 377789319, 755578637, 151115727, 302231455, 604462910, 120892582, 241785164, 483570328, 
        967140656, 193428131, 386856262, 773712525, 154742505, 309485010, 618970020, 123794004, 247588008, 495176016, 990352031, 198070406, 396140813, 792281625, 158456325, 
        316912650, 633825300, 126765060, 253530120, 507060240, 101412048, 202824096, 405648192, 811296384, 162259277
    };
    
    static int expo10[340] = { // base 10 exponent for powers of 2 in range (-180 to 160) - 52
        -70, -70, -70, 
        -69, -69, -69, -69, -68, -68, -68, -67, -67, -67, -66, -66, -66, -66, -65, -65, -65,
        -64, -64, -64, -63, -63, -63, -63, -62, -62, -62, -61, -61, -61, -60, -60, -60, -60,
        -59, -59, -59, -58, -58, -58, -57, -57, -57, -56, -56, -56, -56, -55, -55, -55, 
        -54, -54, -54, -53, -53, -53, -53, -52, -52, -52, -51, -51, -51, -50, -50, -50, -50,
        -49, -49, -49, -48, -48, -48, -47, -47, -47, -47, -46, -46, -46, -45, -45, -45, 
        -44, -44, -44, -44, -43, -43, -43, -42, -42, -42, -41, -41, -41, -41, -40, -40, -40,
        -39, -39, -39, -38, -38, -38, -38, -37, -37, -37, -36, -36, -36, -35, -35, -35, -35,
        -34, -34, -34, -33, -33, -33, -32, -32, -32, -32, -31, -31, -31, -30, -30, -30,
        -29, -29, -29, -28, -28, -28, -28, -27, -27, -27, -26, -26, -26, -25, -25, -25, -25,
        -24, -24, -24, -23, -23, -23, -22, -22, -22, -22, -21, -21, -21, -20, -20, -20,
        -19, -19, -19, -19, -18, -18, -18, -17, -17, -17, -16, -16, -16, -16, -15, -15, -15,
        -14, -14, -14, -13, -13, -13, -13, -12, -12, -12, -11, -11, -11, -10, -10, -10, -10,
        -9, -9, -9, -8, -8, -8, -7, -7, -7, -7, -6, -6, -6, -5, -5, -5, -4, -4, -4, -4, -3, 
        -3, -3, -2, -2, -2, -1, -1, -1, +0, +0, +0, +0, +1, +1, +1, +2, +2, +2, +3, +3, +3, 
        +3, +4, +4, +4, +5, +5, +5, +6, +6, +6, +6, +7, +7, +7, +8, +8, +8, +9, +9, +9, +9, 
        +10, +10, +10, +11, +11, +11, +12, +12, +12, +12, +13, +13, +13, +14, +14, +14, 
        +15, +15, +15, +15, +16, +16, +16, +17, +17, +17, +18, +18, +18, +18, +19, +19, +19, 
        +20, +20, +20, +21, +21, +21, +21, +22, +22, +22, +23, +23, +23, +24, +24, +24, +24, 
        +25, +25, +25, +26, +26, +26, +27, +27, +27, +27, +28, +28, +28, +29, +29, +29, 
        +30, +30, +30, +31, +31, +31, +31, +32
    };
    
    double *pf = &f;
    uint64_t *fbits = (uint64_t *) pf;
    int ee = (*fbits >> 52) & 0x7FF;                // exponent bits
    uint64_t mm = (1LL << 52) + (*fbits & 0xFffffFFFFffff);     // mantissa bits
    if (ee == 0x7FF || ee < 1023 - 180 || ee >= 1023 + 160) {
        float v = (float) f;                        // special or out of range, convert to float
        return *(decims32 *)(&v);
    }
    mm = (mm + 500000) / 1000000;                   // convert to millions
    mm = powersOf2[ee - 1023 + 180] * mm;       
    uint32_t x = (mm + 5000000000) / 10000000000;   // divide by ten billion to get a value in range of 10,000,000
    int e = expo10[ee - 1023 + 180] + 15;           // TODO: why 15?
    while (e < -48 || x >= 100000000) {
        x /= 10;
        ++e;
    }
    return makeNumber32_(*fbits >> 63, x, e);
}

//__attribute__((always_inline))
int numberParts32_(decims32 num, int * expn, uint32_t * decimals) {
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

double numberAsDouble32(decims32 num) {
    static double ef[] = { 1e-54, 1e-53, 1e-52, 1e-51, 1e-50,
        1e-49, 1e-48, 1e-47, 1e-46, 1e-45, 1e-44, 1e-43, 1e-42, 1e-41, 1e-40,
        1e-39, 1e-38, 1e-37, 1e-36, 1e-35, 1e-34, 1e-33, 1e-32, 1e-31, 1e-30,
        1e-29, 1e-28, 1e-27, 1e-26, 1e-25, 1e-24, 1e-23, 1e-22, 1e-21, 1e-20,
        1e-19, 1e-18, 1e-17, 1e-16, 1e-15, 1e-14, 1e-13, 1e-12, 1e-11, 1e-10,
        1e-9, 1e-8, 1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1,
        1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
        1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
        1e20, 1e21, 1e22, 1e23, 1e24, 1e25, 1e26, 1e27, 1e28, 1e29,    
        1e30, 1e31, 1e32, 1e33, 1e34, 1e35, 1e36, 1e37, 1e38, 1e39,
        1e40, 1e41, 1e42, 1e43, 1e44, 1e45, 1e46, 1e47
    };
    int expn;
    uint32_t mant;
    int negat = numberParts32_(num, &expn, &mant);
    return (negat ? -mant : mant) * ef[expn + 54] * 0.0000001;
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
    sprintf(result + 11, "%+d", expn);
    return result;
}

//__attribute__((always_inline))
uint32_t shiftDecimals32(uint32_t decimals, int amount) {
    if (amount > 8 || decimals == 0)
    {
        return 0;
    }
    switch (amount % 4)
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
    if (amount >= 4 && decimals != 0)
    {
        decimals /= 10000;
    }
    if (amount == 8 && decimals != 0)
    {
        decimals /= 10000;
    }
    return decimals;
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
        small = shiftDecimals32(small, exp_diff);
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
        while (expn != -48 && prod < 10000000)
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
        //printf("div32 loop %de%d / %de%d quo=%llu exp=%d\n", m_a, exp_a, m_b, exp_b, quo, expn);
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
        if (expn == -48) break;       // subnormal number
        quo *= 10;
        --expn;
    }
    return makeNumber32_(negat, quo, expn);
}

int main(int n, char * args[]) {
    printf("0.0276840e-48: %s\n", numberAsString32(asDecimal32(0.0276840e-48)));
    printf("Largest number: %s\n", numberAsString32(0x7F7D7840));  // 5.0e+47
    printf("Smallest normal: %s\n", numberAsString32(0xF4240));    // 1.0e-48
    printf("Eight significant digits: %s\n", numberAsString32(0x1C000000));  // Smallest 8-digit precision 1.0e-6
    printf("Seven significant digits: %s\n", numberAsString32(0x1A000000 + 29999999));  // Largest small 7-digit precision 9.999,999e-7
    printf("Smallest number (1.0e-54): %s\n", numberAsString32(0x00000001));
    printf("Number 4.698,543e10: %s\n", numberAsString32(asDecimal32(+4.698543e+10)));
    printf("Invalid raw value: %s\n", numberAsString32(20000000));    // Not a number (missing number in units place)
    printf(" +∞: %s\n", numberAsString32(0x7f800000));
    printf(" -∞: %s\n", numberAsString32(asDecimal32(-5.1658240e+47)));
    printf("Zero: 0x%.8x \n", asDecimal32(0.0));        // 0
    printf("One: 0x%.8x \n", asDecimal32(+1.0));        // 0x40000000
    printf("Four: 0x%.8x \n", asDecimal32(+4.0));       // 0x42000000
    printf("Minus five dot five: 0x%.8x \n", asDecimal32(-5.5));    // 0xc2e4e1c0
    printf(" 1.0e-47 + 0.123456e-48 = %s\n", numberAsString32(add32(asDecimal32(1.0e-47), 123456)));
    printf(" 1.0000009 + 1.0000004 = %s\n", numberAsString32(add32(asDecimal32(1.0000009), asDecimal32(1.0000004))));
    printf(" -1.0e-48 + 0.123456e-48 = %s\n", numberAsString32(add32(asDecimal32(-1.0e-48), 123456)));
    printf(" -1.0 - -1.2345678e-6 = %s\n", numberAsString32(sub32(asDecimal32(-1.0), asDecimal32(-1.2345678e-6))));
    printf(" 1.0000009e4 - 1.0000004e4 = %s\n", numberAsString32(sub32(asDecimal32(1.0000009e4), asDecimal32(1.0000004e4))));
    printf(" 3.0e3 * 5.0 = %s\n", numberAsString32(mul32(asDecimal32(3.0e3), asDecimal32(5.0))));
    printf(" -1.0e-48 * 0.123456e-48 = %s\n", numberAsString32(mul32(asDecimal32(-1.0e-48), 123456)));
    printf(" -10.0 * 0.123456e-48 = %s\n", numberAsString32(mul32(asDecimal32(-10.0), 123456)));
    printf(" 3.0e3 / 5.0 = %s\n", numberAsString32(div32(asDecimal32(3.0e3), asDecimal32(5.0))));
    printf(" -1.0e-48 / 0.123456e-48 = %s\n", numberAsString32(div32(asDecimal32(-1.0e-48), 123456)));
    printf(" -3.0e-2 / 0.006e-48 = %s\n", numberAsString32(div32(asDecimal32(-3.0e-2), 6000)));
    printf(" 9.999999e-48 / 1.0e-54 = %s\n", numberAsString32(div32(asDecimal32(9.999999e-48), 1)));
    static double ef[] = { 1e-54, 1e-53, 1e-52, 1e-51, 1e-50,
        1e-49, 1e-48, 1e-47, 1e-46, 1e-45, 1e-44, 1e-43, 1e-42, 1e-41, 1e-40,
        1e-39, 1e-38, 1e-37, 1e-36, 1e-35, 1e-34, 1e-33, 1e-32, 1e-31, 1e-30,
        1e-29, 1e-28, 1e-27, 1e-26, 1e-25, 1e-24, 1e-23, 1e-22, 1e-21, 1e-20,
        1e-19, 1e-18, 1e-17, 1e-16, 1e-15, 1e-14, 1e-13, 1e-12, 1e-11, 1e-10,
        1e-9, 1e-8, 1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1,
        1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
        1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
        1e20, 1e21, 1e22, 1e23, 1e24, 1e25, 1e26, 1e27, 1e28, 1e29,    
        1e30, 1e31, 1e32, 1e33, 1e34, 1e35, 1e36, 1e37, 1e38, 1e39,
        1e40, 1e41, 1e42, 1e43, 1e44, 1e45, 1e46, 1e47
    };
    /*
    for (int expn = -48; expn <= 47; ++expn)
    {
        for (int units = -9; units <= 9 - 5 * (expn == 47); units += 1 + (expn > -48 && units == -1))
        {
            long r = random();
            long s = r;
            for (int deci = 0; deci < 10000000; deci += 1 + r & 7)
            {
                double f = ((units + deci * 0.00000001) * ef[expn + 54]) + ((double) s * r);
                if ((random() & 0xff7b6064) == 0x2d586024) printf(" %g ", f);
                r >>= 3;
                if (r == 0) r = random();
            }
            puts("");
        }
    }
    */
    
    printf("3.437519e22 = %.8g = %s\n", (3 + 4375190 * 0.0000001) * ef[22 + 54], numberAsString32(asDecimal32(3.437519e22)));
    long s = 90000790;
    printf("9.000079e-48 = %.8g\n", s * ef[6] * 0.0000001);
    for (int expn = -48; expn <= 47; ++expn)
    {
        for (int units = -9; units <= 9 - 5 * (expn == 47); units += 1 + (expn > -48 && units == -1))
        {
            long r = random();
            //long s = r;
            for (int deci = 0; deci < 10000000; deci += 3 + r % 8)
            {
                double f = (units * 10000000 + (units >= 0 ? deci : -deci)) * ef[expn + 54] * 0.0000001;
                decims32 fnum = asDecimal32(f);
                decims32 num = makeNumber32_(units < 0, abs(units) * 10000000 + deci, expn); //div32(s * r, );, s * r
                if ((r & 0xff764) == 0x2d524) {
                    printf(" %s«%c» ", numberAsString32(num), 'x' + (num == fnum));
                    if (num != fnum) {
                        int e;
                        uint32_t x;
                        numberParts32_(fnum, &e, &x);
                        printf("%.8g Decomposition: %u exp %d\n", f, x, e);
                    }
                }
                r >>= 3;
                if (r == 0) r = random();
            }
            puts("");
        }
    }
    return s;
}
