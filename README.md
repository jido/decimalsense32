# decimalsense32
A 32-bit decimal number format that makes sense

This project takes some of the ideas of [decimalsense numbers](https://github.com/jido/decimalsense) to the single precision decimal number world.

Decimal numbers do not need be that complex.

Numbers
=======

Normal number range:

**1.0e-48** to **5.0e+47**

(hexadecimal _F4240_ to _7F7D7840_)

Subnormal number range (non-zero):

**1.0e-54** to **9.999,99e-49**

(hexadecimal _1_ to _F423F_)

Normal numbers between 1.0e-6 and 1.0e+6 have eight significant digits. Normal numbers outside that range have seven significant digits. In monetary terms, that means you can only count cents for amounts less than a million.

Zero:

> hexadecimal _0_

Negative zero:

> hexadecimal _80000000_

One:

> hexadecimal _40000000_

Decimalsense numbers are _monotonic_, the unsigned part is ordered when cast as integer, and _unique_: 
there is only one way to encode a particular nonzero number.

According to IEEE, positive and negative zero are equal which is the only exception to unique number representation.

Not a Number (NaN) can have several representations in accordance with IEEE 754. Representation of infinity also follows the IEEE 754 standard.

Format
======

~~~
seeeeeem mmmmmmmm mmmmmmmm mmmmmmmm
~~~

   `s` = sign bit
   
   `e` = 6 exponent bits
      
   `m` = 25 mantissa bits
   
Normal numbers
--------------

The exponent bits dictate how to calculate the exponent and the mantissa.

### Exponent range

* __e = 0–13__

  Decimal numbers in range **1.0e-48** to **9.999,999e-7**

  Exponent = (_exponent bits_ - 16) * 3 + most significant digit of _mantissa bits_
  
  Mantissa = seven least significant digits of _mantissa bits_
  
  Example:
  
  ~~~
  Exponent bits: 6
  Mantissa bits: 1 2345678
  ~~~
  
  Exponent = -30 + 1 = -29\
  Mantissa = 2,345,678
  
  Number = 2.345,678e-29

* __e = 14–49__

  Decimal numbers in range **0.000,001** to **999,999.99**
  
  Exponent = (_exponent bits_ - 14) / 3 - 6
  
  Bucket = remainder of (_exponent bits_ - 14) / 3
  
  Mantissa = _mantissa bits_ +
  -   10,000,000 for bucket 0
  -   40,000,000 for bucket 1
  -   70,000,000 for bucket 2

  Example:
  
  ~~~
  Exponent bits: 36
  Mantissa bits: 123
  ~~~
  
  Exponent = 22 / 3 - 6 = 1\
  Bucket = 1\
  Mantissa = 40,000,000 + 123 = 40,000,123
  
  Number = 4.000,012,3e+1

* __e = 50–63__

  Decimal numbers in range **1,000,000** to **5e+47**
  
  Exponent = (_exponent bits_ - 48) * 3 + most significant digit of _mantissa bits_
  
  Mantissa = seven least significant digits of _mantissa bits_

  Same principles as exponent range __e = 0–13__
 
Subnormal numbers
-----------------

 Encoded in the same way as normal numbers with e = 0, except that m is less than 1,000,000.

 Equivalent to 32-bit integers (ignoring the sign bit) from _0_ to _999,999_
