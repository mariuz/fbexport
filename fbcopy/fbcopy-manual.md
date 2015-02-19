
# FBCopy manual

  

    
    fbcopy {D|C|A|S|X}[EKNHF1234] {source} {destination}  
      
    Source and destination format is [user:password@][host:]database[?charset]  
      
    Options:  
    D  Define - outputs a definition of fields in format:  
       table:common fields:missing fields:extra fields[:where clause|UID]  
    C  Copy - reads that definition from stdin and does the copying  
    S  Single step (define&copy all tables in single step)  
    A  Alter - outputs ALTER TABLE script for fields missing in destination  
    X  Compare data in tables (reads definition from stdin), optionally show:  
       1 same rows, 2 missing rows, 3 extra rows, 4 different rows  
    E  Everything in single transaction (default = transaction per table)  
    K  Keep going (default = stop on first record that cannot be copied)  
    F  Fire triggers (default = temporary deactivate triggers)  
    N  Nulls - used with A. Doesn't put NOT NULL in ALTER TABLE statements  
    H  Html  - used with D, A, X. Outputs differences in HTML format  
    Options are not case-sensitive.  
    

  
  
Usage and examples

  
The main idea behind FBCopy is that you have two databases and wish to copy
data from one to another. These databases can have a different structure, so
you should be careful which tables and fields to include in copy. Also, the
data is referenced with foreign keys or check constraints, so you want to be
sure that the order of inserting is right. You might also have triggers that
alter the data upon inserting, so you might need to disable them temporarily.
FBCopy takes care of all these things so you don't make mistakes.

Newer versions of FBCopy can also compare data in databases.

  
Examples use default sysdba/masterkey username and password, and copy data
from database /dbases/employee.fdb to database /dbases/test.fdb.

  
  
  
  
Step1: Determine the differences between databases

  
  
fbcopy D sysdba:masterkey@localhost:/dbases/employee.fdb
sysdba:masterkey@localhost:/dbases/test.fdb

  
If you have ISC_USER and ISC_PASSWORD environment variables, they will be
used, so you can just type:

  
fbcopy D localhost:/dbases/employee.fdb localhost:/dbases/test.fdb

  
Also, if you wish, you can use direct connection on local server (or with
embedded server):

  
fbcopy D /dbases/employee.fdb /dbases/test.fdb

  
The D command will output list of all tables in source database, in the
following format:

  
table:list of columns common to both tables:list of columns missing in
destination table: list of extra columns in destination (missing from source)

  
Columns are separated with commas, the output will look something like this:

  

COUNTRY:COUNTRY,CURRENCY::

JOB:JOB_CODE,JOB_COUNTRY,JOB_GRADE,JOB_REQUIREMENT,JOB_TITLE,LANGUAGE_REQ,MAX_
SALARY,MIN_SALARY::

EMPLOYEE:DEPT_NO,EMP_NO,FIRST_NAME,FULL_NAME,HIRE_DATE,JOB_CODE,JOB_COUNTRY,JO
B_GRADE,LAST_NAME,PHONE_EXT,SALARY::

PROJECT:PRODUCT,PROJ_DESC,PROJ_ID,PROJ_NAME,TEAM_LEADER::

EMPLOYEE_PROJECT:EMP_NO,PROJ_ID::

PROJ_DEPT_BUDGET:DEPT_NO,FISCAL_YEAR,PROJECTED_BUDGET,PROJ_ID,QUART_HEAD_CNT::

SALARY_HISTORY:CHANGE_DATE,EMP_NO,NEW_SALARY,OLD_SALARY,PERCENT_CHANGE,UPDATER
_ID::

  
You'll need to use this output as input for the actual copying (C option), so
redirect it to some file:

  
fbcopy D /dbases/employee.fdb /dbases/test.fdb > file.def

  
Now, you can edit the .def file with some editor, remove tables (and fields)
you don't want to copy. Also, inspect any differences you find, since you may
need to update some of the databases manually. You can also add a where clause
to the end of each row in .def file, like this:

  
COUNTRY:COUNTRY,CURRENCY::where country = 'Germany'

  
When doing copying, FBCopy only cares about three fields in input it gets:

  * first field (table name)
  * second field (list of table columns)
  * last field (optional where clause)
Therefore, you can simply write:

  
COUNTRY:COUNTRY,CURRENCY:where country = 'Germany'

  
You can also write definition files yourself, or use some other program to do
it. They are simple textual files with colon separated fields.

  
When you just want to view the differences, you can also use HTML output which
is much cleaner and easier to human eye. Just add **H** and redirect to some
html file:

  
fbcopy DH /dbases/employee.fdb /dbases/test.fdb > differences.html

  
Then open the file in some browser and enjoy. You can also use **H** with
**A** option (AH).

Examples: [HTML output with D option](exampled.html), [HTML output with A
option](examplea.html).

  
  
  
  
Step2: Copy data

  
  
To copy the data, you need to send the definition file to FBCopy. It will read
it, and copy all data according to it. FBCopy reads data from standard input,
so you have to pass the file to it:

  
fbcopy C /dbases/employee.fdb /dbases/test.fdb < file.def

  
You can also use the pipes to send definition to FBCopy, example for Windows:

  
type file.def | fbcopy C /dbases/employee.fdb /dbases/test.fdb

  
For Linux:

  
cat file.def | fbcopy C /dbases/employee.fdb /dbases/test.fdb

  
For each line in definition (i.e. for each table), FBCopy starts transaction,
creates INSERT and SELECT statements and copies the data. On any errors, the
transaction is rolled back, so you're sure that either all data is copied to
destination table or none. If you wish, you can run the entire copy (all
tables) in a single transaction. To do that use the E switch:

  
fbcopy CE /dbases/employee.fdb /dbases/test.fdb < file.def

  
Sometimes you may wish to just copy some data, and don't care if some of it
does not get copied. You just need to populate some tables, no matter if some
records are already there. During default operation, FBCopy would stop for
each record that violates primary key, foreign key, or any other constraint.
However, you can use option K to keep going and copy all data that can be
copied, ignoring the records that can't:

  
fbcopy CK /dbases/employee.fdb /dbases/test.fdb < file.def

  
  
  
Single step: Define and copy at once

  
  
There are cases when you know that the databases are the same, and you just
want to copy all data. This can happen, for example, when you have a subset of
data into some temporary database, and you want to import all that into some
master database. For this, use S switch:

  
fbcopy S /dbases/employee.fdb /dbases/test.fdb

  
Of course, you can use the piping to do it with D and C commands too, so S is
really just a shortcut:

  
fbcopy d /dbases/employee.fdb /dbases/test.fdb | fbcopy c /dbases/employee.fdb
/dbases/test.fdb

  
If you are good with pipes, you can write oneliners to copy data of a single
table. Suppose we can to copy data in EMPLOYEE table:

  
fbcopy d /dbases/employee.fdb /dbases/test.fdb | grep EMPLOYEE | fbcopy c
/dbases/employee.fdb /dbases/test.fdb

  
  
  
  
Additional feature: Building the ALTER TABLE and CREATE TABLE script

  
  
Sometimes, you wish to copy data, but you're missing some fields in some
tables of destination database. You need to add them, but it's a lot of work:
loading some admin. tool, determine column types, and add to destination.
FBCopy can do this hard work for you.

  
fbcopy A /dbases/employee.fdb /dbases/test.fdb > changes.sql

  
This will output a regular .sql script with ALTER TABLE statements to add
fields. Also, for tables missing in destination, it will create CREATE TABLE
statements. It only creates datatype, not-null option and primary keys. If you
have checks, foreign keys, indices, triggers, or something else on those
tables, you have to create it manually (This may be improved in future
versions of FBCopy).

  
When you get a ALTER TABLE statement, there is a chance that you already have
some data in destination database. So, adding a NOT NULL column can be a
problem. Therefore, there is an option N, to avoid NOT NULLs for ALTER TABLE
statements:

  
fbcopy AN /dbases/employee.fdb /dbases/test.fdb > changes.sql

  
  
  
  
New in version 1.50: Compare data

  
  
You can use FBCopy's **X** option to compare data in databases. It compares
the same table in two different databases, and you can use it to validate that
copying went fine. This option always requires the list of tables and fields
to compare via stdin. So, first build a list with **D** option:

  
fbcopy D /dbases/employee.fdb /dbases/test.fdb > file.def

  
Now, run the comparer:

  
fbcopy X /dbases/employee.fdb /dbases/test.fdb < file.def

  
It will write each table name, and four columns representing the number of
records that are the same, different, those that are missing in source and
missing in destination database. By _same_ it means that all columns supplied
in column list have the same values (NULL is treated equal to NULL in this
case). Rows are compared based on the primary key value. If values of primary
key columns from one database are not found in other, than that row is treated
as _missing_. Rows that have same values for primary key columns, but
different values for other columns are considered _different_. Current version
can only compare tables with primary keys, but support for tables with unique
indices might be introduced soon. In some future versions there will be
ability to compare any tables regardless of the primary key presence.

  
You can also use H option to get the HTML table with same columns.

  
fbcopy XH /dbases/employee.fdb /dbases/test.fdb < file.def > comparison.html

  
After this, you might also want see the actual records that are same, differ
or missing. This output is only available in HTML format. To see same rows,
add 1 to flags. To see rows missing in destination, add 2. To see rows missing
in source add 3. To see different rows, add 4. Different rows have split-cells
for values that differ. Example that shows all the rows and colors them
appropriately:

  
fbcopy XH1234 /dbases/employee.fdb /dbases/test.fdb < file.def > records.html

  
Example that only shows rows missing from destination database:

  
fbcopy XH2 /dbases/employee.fdb /dbases/test.fdb < file.def > records.html

  
Of course, if you only want to see rows of a single table (or selected
tables), edit the file.def file before running the command.

  
Here are some examples of data comparison: [overview](examplex.html),
[detailed view of records](examplex2.html)

  
  
  
If you have any suggestions or remarks, please contact me.

  

* * *

Copyright (C) 2005, 2006 Milan Babuškov.
([e-mail](mailto:mbabuskov@yahoo.com))

  
  

[Home](/)

[FBExport](/fbexport.php)

[FBCopy](/fbcopy.html)

[Nagios plugin](/nagios.html)

[Project page](http://sourceforge.net/projects/fbexport)

[Download area](http://www.firebirdfaq.org/fbcopy.php)

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

