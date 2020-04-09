fbexport
========

FBExport is a tool for importing/exporting data and executing SQL statements on Firebird , FBCopy is a tool to copy and compare data accross Firebird databases.


FBExport

export and import data from Firebird databases
command line and GUI version
runs on Windows and Linux
export to comma separated values (CSV) format
export as INSERT statements
use exported data in DML statements
handles NULLs and BLOBs properly
ability to execute sql scripts from a file

FBCopy

copy and compare data between Firebird databases
runs on Windows and Linux
automatically loads tables and compares their fields
uses Foreign Keys and Checks to determine the correct order of tables
can create ALTER TABLE and CREATE TABLE scripts needed to update the destination database
HTML overview of differences in data and metadata


Export client data
```
docker build --build-arg DATEBASE_NAME_ARG=DATABASE_NAME.FDB -t firebirdexport -f Dockerfile.init .   
docker run -v DATABASE_PATH:/var/lib/firebird/2.5/data -it --entrypoint /bin/bash firebirdexport
```

## Development (blp-digital)

The test and deploy process is manual at the moment since we do not antecipate frequent changes.

Test:

```bash
bash -x integration_test.sh
```

Build and publish:

```bash
gcloud builds submit --config cloudbuild.yaml .
```
