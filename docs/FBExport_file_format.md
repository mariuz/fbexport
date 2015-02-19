# FBExport

Tool for exporting and importing data with Firebird and InterBase databases

  
FBExport file format

  
In case anyone wants to know, or (s)he needs to read/write fbx file from other
programs.

I did my best to show this correctly. If you don't get something to match the
real file, take a look at the code, or e-mail me.

  
  
Structure of the file:

  

Byte

Length

Value

Explained

0

1

0

Always zero

1

1

125

FBExport file version, currently: 125

2

1

FC

Field count (number of columns)

3

FC

TYPE

Column types

The following part repeats (except for BLOBs which don't have it at all)

FC + 4

1

length

Length of the next data in bytes (read below)

FC + 5

length

data

Data itself

End of file

  

  
  
Column types:

  

Type code

IBPP Name

SQL Name

0

Array

Array

1

Blob

Blob

2

Date

Date

3

Time

Time

4

Timestamp

Timestamp (date in Dialect 1)

5

String

Char, Varchar

6

Smallint

Short

7

Integer

Int

8

LargeInt

Decimal(18, 0) only in Dialect 3

9

Float

Float

10

Double

Double precision and Decimal/Numeric with scale

  
  
BLOBs are special case, they are written like this:

Byte

Length

Value

Explained

0

1

0 or 1

0 when column is NULL, 1 when it is not NULL. (hex values 0x00 and 0x01)

The following bytes repeat until the end of BLOB data

1

4

xxxx

size of next segment (0-8192) converted to string

Example: length of 1280 is written as 0x31 0x32 0x38 0x30

xxxx + 5

xxxx

DATA

Blob data (one segment)

  
  
Length:

  
Possible values range from 0 to 255. Value of 255 marks the NULL value. To
keep the file small, I only use 1 byte for length since it is quite enough for
most values. But sometimes, there are char columns larger than 254 bytes, so
more than one byte is needed to represent the length. In such cases two
additional bytes are used. Here's the final explanation:

  

Length byte value

Meaning

0-253

Single byte length, just read it.

254

Special mark, the following two bytes represent the real length of data (byte1
* 256 + byte2)

255

NULL value

  
Example: Timestamp 21. July 1997 07:30:00 is stored in file as string
19970721073000. Length of that string is 14 bytes, so value 14 (hex: 0e) is
written for the length.

  
  
Data:

  
All data is written as readable ASCII characters, except char/varchar which
are written as they are (if they contain any non-ASCII character, those will
be used). All numers are converted to strings. I know it is not optimal, but
it is human readable and compresses very good.

  

IBPP Type

Written as

Array

_- not yet supported by FBExport -_

Blob

special case

Date

string representation of number of days since 1.1.1900

Time

string representation of number of seconds since midnight

Timestamp

YYYYMMDDhhmmss

String

as is

Smallint

as string

Integer

LargeInt

Float

as string, dot (.) is used as decimal separator

Double

  
  
If you have any suggestions or remarks, please contact me.

  

* * *

Copyright (C) Milan Babuškov 2002, 2003, 2004.
([e-mail](mailto:mbabuskov@yahoo.com))

