# FBExport

Tool for exporting and importing data with Firebird  databases

**What's new?**

*   FBExport and FBCopy sources are joined and built from the same place
*   Fixed bug when copying blobs with Firebird 2.1
*   Slash character is no longer removed when loading statements from file
*   Fixed bug when exporting negative values like -0.01
*   Added header with column names to CSV export
*   Username and password are read from ISC_USER and ISC_PASSWORD enviroment variables if available
*   You can fetch the latest source code from Git repository
*   Read [HTML manual](manual.html) for usage instructions

**A note to first-time users**

FBExport is a tool that can **export** data in CSV format, as SQL script
of INSERT statements, or a custom binary .fbx format. However it can only
**import** this custom .fbx format. (Well, actually it can import the
INSERT statement as well, via -Q -F parameter combination). 

If you
are looking for a tool to **import** data like CSV or XML file into Firebird
you should use some other tool like [Guacosoft XMLWizard](http://www.guacosoft.com/xmlwizard) or something
similar.

**What does it do?**

With FBExport, one can easily
export/import data from/to Firebird table into a text file (actually
it's a binary file). I first tought that XML format would be the way to
go, but size of exported file matters, so I used [my own format](FBExport_file_format.html) which uses less
space. It can also export data in CSV format and as INSERT statements.

FBExport is a command-line
tool, so you can easily use it inside batch scripts. Currently, Linux
and Windows versions are available, but I hope it can be built with any
compiler that can build IBPP library. For versions 1.40 to 1.60, the is
also a GUI version of the tool available. GUI is not flexible enough to
support the newest features in version 1.65, so 1.60 will probably be
the last version with GUI variant.

FBExport is released as a single package containing
source code, linux and windows command-line
and GUI binaries.

**Why use it?**

I was trying to copy data from one
[Firebird](http://www.firebirdsql.org) database to another
via external tables. The whole import/export process was needed since
there was no chance of getting access to both databases at the same
time, so any of those data-pump utilities was out of the question.
Thus, I came up with the idea to export data into text file, copy that
file to other server, and import the data into other database.

Then I had problem with NULLs. When exporting data into external tables, database
engine turns all null values into zeroes (hex value 0). When importing
back, integers become zero, dates become 17.11.1858, etc.
I also tried to use CHAR for all exporting, but then, null value for
integer becomes "&nbsp;&nbsp;&nbsp;&nbsp;", or
something similar, and one gets error message: _Conversion error
from string "&nbsp;&nbsp;&nbsp;&nbsp;"_.

Instead of writing some UDFs
or a lot of Stored Procedures for each table, I decided to make a tool
that will do this job...

Also, Firebird has a heavy network protocol which **works slow over
the modem line**, so it takes a lot of time to send data with some
data-pump tool. With FBExport you can write a batch script to export
the data at one end, zip it, and send over the wire. Then at the other
side, unpack and import. Transfering one big .zip file is much faster
than making database connections, transactions, filling parameter
values, executing statements, commiting, etc. 

**Restrictions**

This tool uses [IBPP library](http://ibpp.sourceforge.net/)
for database access. It supports Firebird 1.0, 1.5, 2.0, 2.1, 2.5 
databases. 

FBExport doesn't support ARRAY datatype.

**Licence**

FBExport is Free Software, released under MIT/Expat licence.
