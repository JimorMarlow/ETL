#pragma once
// etl_version.h - version control

// NOTE: after version changing run script in terminal: 
// > python sync_version.py
// and config files: library.json, library.properties, package.json will be patched...

#define ETL_VERSION_MAJOR 0
#define ETL_VERSION_MINOR 9
#define ETL_VERSION_PATCH 2

// Хелпер для stringify
#define ETL_STRINGIFY_HELPER(x) #x
#define ETL_STRINGIFY(x) ETL_STRINGIFY_HELPER(x)

//Собрать строку версии "X.X.X"
#define ETL_VERSION_STRING \
   ETL_STRINGIFY(ETL_VERSION_MAJOR) "." \
   ETL_STRINGIFY(ETL_VERSION_MINOR) "." \
   ETL_STRINGIFY(ETL_VERSION_PATCH)