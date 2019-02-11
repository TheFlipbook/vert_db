# vert_db

A header-only C++ library for operating on 3d model vertex data.
* Spatial and connectivity queries
* Transferring properties like skin weights

## Getting Started (Using)
1. Drop files from /include/ into somewhere your project can see them
2. include "vert_db/vert_db.h" and use vert_db for basic query functionality
3. include "vert_db/transfer_utils.h" and use transfer_db for doing transforms on data.
4. Check out /tests/. for interesting use cases.

## Getting Started (Developing)
1. Download and install premake5 (https://premake.github.io/)
2. Download Catch2 (https://github.com/catchorg/Catch2) and place the header-only version in /external/.
3. Run premake.bat
4. Open /_build/vert_db.sln and compile solution.
5. Run /_Bin/(config)/(platform)/bin/vert_db-test.exe to validate changes.
