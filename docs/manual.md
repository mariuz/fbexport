# FBExport

Tool for exporting and importing data with Firebird and InterBase databases

  
FBExport manual

  

* Basic usage

  * export data from database to .fbx file
  * import data from .fbx file to database
  * execute sql statement
  * show list of connected users
* Advanced usage  

  * pump data from one to other database
  * updating instead of inserting
  * running large SQL scripts  

* Other formats

  * export as CSV
  * export as insert statements
  * formatting date and time values  

  
Basic usage

  
All examples use local server and database located at c:\dbases\test.gdb.
Default sysdba/masterkey login is used.

Command-line options are not case sensitive, but are written in upper case for
better visibility. Also, the argument for options don't need to be separated,
but they are for the same reason.

  
  

Exporting data from table "mytable" to file "myfile.fbx"

  

fbexport -S -H localhost -D c:\dbases\test.gdb -U sysdba -P masterkey -F
myfile.fbx -Q "SELECT * FROM mytable"

  
The host defaults to LOCALHOST and username to SYSDBA, so you can write just:

  

fbexport -S -D c:\dbases\test.gdb -P masterkey -F myfile.fbx -Q "SELECT * FROM
mytable"

  
If you wish to use direct local connection (without TCP/IP), you have to set
hostname to nothing like this: -H ""

  
WARNING: I don't recommend using SELECT * as your select statement while
exporting. Column order is unpredictable and information about columns' names
is not stored in export-file. Please use column list like this:

  

-Q "SELECT c1, c2, ... FROM mytable"  

  
You can use any valid SELECT statement to export data (where clause, joins,
anything...). However if you wish to export all columns of single table, the
easiest option is to use -V switch. FBExport will build the list of column in
alphabetical order so you don't have to worry about it

  
Using verbatim copy options

  

fbexport -S -D c:\dbases\test.gdb -P masterkey -F myfile.fbx -V mytable

  
If you don't want to export the entire table, you can specify where clause
with -Q switch, like this:

  

fbexport -S -D c:\dbases\test.gdb -P masterkey -F myfile.fbx -V mytable -Q
"where x = 10"

  
When exporting data of char datatype, the field values are padded with blanks
to full length, so you can use -T option to trim them. It is also good option
when exporting into CSV format, since you may want to import it somewhere else
and it may look ugly.

  

  
  

Import data from file "myfile.fbx"&nbsp_place_holder; to table "mytable"

  

fbexport -I -H localhost -D c:\dbases\test.gdb -U sysdba -P masterkey -F
myfile.fbx -Q "INSERT INTO mytable (col1, col2, ...)"

  
Same as with exporting: host defaults to localhost, username to sysdba, and
you can use -V switch if you're filling up all columns:

  

fbexport -I -D c:\dbases\test.gdb -P masterkey -F myfile.fbx -V mytable

  
When importing, you may want to use -R option to make import an atomic
operation. When you use -R option, the transaction is rolled back if any
errors happen, so either all data is imported or none.

  
Also, you can control number of errors you wish to allow during import. Use -E
option for that. For example if you know that some 10 rows of data cannot be
inserted:

  
&nbsp_place_holder;&nbsp_place_holder;&nbsp_place_holder; fbexport -I -D
c:\dbases\test.gdb -P masterkey -F myfile.fbx -V mytable -E 10

  
Default value is zero (don't allow any errors). If you wish to allow unlimited
number of errors, use value -1 (-E -1).

  
  

  

Executing SQL statement

  

fbexport -X -D c:\dbases\test.gdb -U sysdba -P masterkey -Q "DELETE FROM
mytable"

  
Unlike isql, fbexport returns non-zero value if statement fails, so you can
use it in maintenance scripts. From version 1.60 you can use it together with
-F to load statements from sql file, and execute multiple statements with
error control.

  

fbexport -X -F script.sql -D c:\dbases\test.gdb -U sysdba -P masterkey -Q
"DELETE FROM mytable"

  
  

  
  

Show list of connected users

  

fbexport -L -H localhost -D c:\dbases\test.gdb -P masterkey

  
This command requires server that provides Services API, so it may not work on
Classic server. It just lists names of users that are connected to database.

  

  
Advanced usage

  
  

Pump data from one to other database

  
You can pump data with fbexport between two databases by exporting and
importing. For example, let's copy all records of table mytable from server1,
database1.gdb to server2, database2.gdb

  

fbexport -S -V mytable -D database1.gdb -H server1 -U username -P password -F
mytable.fbx

fbexport -I -V mytable -D database2.gdb -H server2 -U username -P password -F
mytable.fbx -R

  
Even better, since FBExport can read/write standard input/output, it can be
done directly, without intermediate file (mytable.fbx). Simple piping:

  

fbexport -S -V mytable -D db1.gdb -H server1 -P masterkey -F - | fbexport -I
-V mytable -D db2.gdb -H server2 -P masterkey -F - -R

  
  

  

Updating instead of inserting

  
From version 1.55, FBExport supports ordered parameters which can be directly
named via -Q option. To use this, when importing, you should use switch -If
instead of just -I. Usage examples:

  
Export data:

  

fbexport -S -D c:\dbases\test.gdb -P masterkey -F my.fbx -Q "SELECT one, two,
three FROM mytable"

  
Regular import:

  

fbexport -I -D c:\dbases\test2.gdb -P masterkey -F my.fbx -Q "INSERT INTO
mytable (one, two, three)"

  
Parametrized import:

  

fbexport -If -D c:\dbases\test2.gdb -P masterkey -F my.fbx -Q "INSERT INTO
mytable (one, two, three) values (:1, :2, :3)"

fbexport -If -D c:\dbases\test2.gdb -P masterkey -F my.fbx -Q "INSERT INTO
mytable (two, three, one) values (:2, :3, :1)"

  
Parametrized update

  

fbexport -If -D c:\dbases\test2.gdb -P masterkey -F my.fbx -Q "update mytable
set two = :2, three = :3 where one = :1"

  
This way you can not only export/import, but also update values between
databases.

  
  

  

Running large SQL scripts

  
If you need to run large scripts of SQL statements, FBExport can be a great
replacement for isql tool, as it has much better error checking. If you are
importing or updating a lot of rows via UPDATE or INSERT statement, you can
use -M option to commit periodically to improve performance

  
Load and run sql script file:

  

fbexport -X -D c:\dbases\test.gdb -P masterkey -F script.sql

  
Commit at each 2000th statement:

  

fbexport -X -D c:\dbases\test2.gdb -P masterkey -F script.sql -C 2000 -M

  
  

  
  
Other formats

  
Besides [default .fbx format](FBExport_file_format.html), data can be exported
as rows of CSVs or INSERTs. CSV is short for Comma Separated Values. Each
database row is output in format:

  

"field_value","field_value","field_value"

  
Values with NULLs are not quoted, example:

  

"field_value",,"field_value"

  
When exporting as insert statements, each row is output like this example:

  

INSERT INTO TABLENAME (COLUMN1, COLUMN2, COLUMN3) VALUES ('field_value',
numeric_value, null);

  
  
Date, Time and Timestamp columns can be customly formatted with J and K
swithches

  
Date format (J switch)

Default format is D.M.Y which is output as 31.01.2000.

  

Format specifiers:

D - day of month 01-31

d - day of month 1-31

M - month 01-12

m - month 1-12

y - year 00-99

Y - year 0000-9999

  
Time format (K switch)

Default format is H:M:S which is output as 23:59:59

  

h - hour 0-23

H - hour 01-23

m - minute 0-59

M - minute 00-59

s - second 0-59

S - second 00-59

  
Notes on date & time formats:

All other characters are output as they are, so you can use colon, slashes,
whatever.

If you wish to output just the date portion of timestamp column, set time
format to "" and fbexport

will remove the trailing space character (between date and time).

  
  
If you have any suggestions or remarks, please contact me.

  

* * *

Copyright (C) Milan Babuškov 2002-2005. ([e-mail](mailto:mbabuskov@yahoo.com))

  

