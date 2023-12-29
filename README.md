### Protocol Overview
General rules of the protocol:
  * Each object should be less or equal size of the MTU of `1500` bytes
  * First octet of each object should contain unique identifier for this object(f.e: `MESSAGE_TYPE_FRAME`)

The `Ozzy::Frame` object consists of:
  * Type
  * Length of the payload
  * Checksum
  * Payload

Type and length are 2 octets, checksum is the 4, Total of 8 octets for the 
frame header. The idea is that we have kinda dynamic checksum size, and if
we want to add more fields to the header(for example 1 bit `is_encrypted` flag)
wi will not drop the payload offset, and just borrow bits from the checksum bitfield.

```
                        Frame header(always 8 octets)
+----------------------+---------------------------+---------------------------+
|       Field          |    Octets (LSB-MSB)         |        Description      |
+----------------------+---------------------------+---------------------------+
|       type           |   0 - 7                     |  Type of this message   |
|                      |                             |                         |
+----------------------+---------------------------+---------------------------+
|      length          |   8 - 15                    |  Maximum length of the  |
|                      |                             |  packet payload.        |
+----------------------+---------------------------+---------------------------+
|     checksum         |   16 - 63                   |  Bits for the checksum  |
|                      |                             |  of the payload.        |
+----------------------+---------------------------+---------------------------+

               Frame payload(1400 octets in this implementation)
+----------------------+---------------------------+---------------------------+
|     payload          |   64 - 1463               |  Payload with doubles.    |
|                      |                           |                           |
+----------------------+---------------------------+---------------------------+
```
