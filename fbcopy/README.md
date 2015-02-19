# FBCopy

Tool for copying and comparing data in Firebird databases

**Features**

*   Copy data between Firebird databases
*   Ability to compare data in databases
*   Can disable triggers to avoid side-effects
*   Understands foreign keys so order of copying is correct
*   Can work in overwrite mode (UPDATE if INSERT fails)
*   Open source under Expat (a.k.a. MIT) license
*   Now you can freely use any part of FBCopy code in your applications
*   Read [HTML manual](fbcopy-manual.html) for usage instructions

**What does it do?**

With FBCopy, you can easily **copy data** between Firebird tables
and databases. FBCopy is a command-line tool, so
you can use it inside batch scripts, cron jobs, etc. Currently,
Linux and Windows versions are available, but I hope it can be built
with any compiler that can build IBPP library. It has been tested and
works on FreeBSD as well.

You can also **compare data** in different databases. It
can output a simple tabular overview of different/same records, or a detailed HTML that
shows each record in both databases. See [manual](fbcopy-manual.html) for details.

**Why use it?**

I was trying to copy data from one
[Firebird](http://www.firebirdsql.org) database to another.
There are a lot of nice GUI tools that do it, but they all require thay
you have fast access to database. Both databases I had were on
remote server, and all I had was
console access via ssh. Databases were too big to copy them to my
machine, pump the data, and copy back. So I had to pump on the server.
There were no tools for this, so I decided to write one.

**Restrictions**

This tool uses [IBPP library](http://ibpp.sourceforge.net/)
for database access. It supports Firebird 1.0, 1.5, 2.0
databases. If it works for you, please let me
know. As IBPP project grows, I hope to support all database engines
they do.

FBExport doesn't support ARRAY datatype yet. If you need it, please let
[me](mailto:mbabuskov@yahoo.com) know. Also, dialect 1 is not
supported at this time, only Dialect 3.

**License**

FBCopy is Free Software, released under Expat (a.k.a. MIT) license

**Download**

Latest version can be downloaded from [Firebird FAQ website](http://www.firebirdfaq.org/fbcopy.php).
