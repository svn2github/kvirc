This file contains all the things that have to be done to complete the
transition autotools -> cmake.

* Missing targets:
- create and test build and install rules for the win32 target

* Missing or broken rules
- doc/*
- po/*

* Translation
- create a make target for updating *.pot/*.po files
- Cleanup: move the install target from cmake/modules/FileGetText
        to a better place

* Code
- several #defines are undefined
-- src/kvilib/config/kvi_settings.h : VERSION, BUILD_DATE, BUILD_FLAGS

