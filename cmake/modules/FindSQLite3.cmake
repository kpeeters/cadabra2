# Try to find SQLite3
# uses:
# SQLITE3_INCLUDE_DIR_SEARCH - guess for includes directory (contains sqlite3.h)
# SQLITE3_LIBRARIES_SEARCH - guess for libraries directory (contains sqlite3.lib)
# sets:
# SQLITE3_FOUND - system has SQLITE3
# SQLITE3_INCLUDE_DIR - Includes directory (contains sqlite3.h) 
# SQLITE3_LIBRARIES - Libraries directory (contains sqlite3.lib)

find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h
			PATHS ${SQLITE3_INCLUDE_DIR_SEARCH}
			DOC "SQLite3 includes directory (contains sqlite3.h)"
			)
find_library(SQLITE3_LIBRARIES NAMES sqlite3.lib
			PATHS ${SQLITE3_LIBRARIES_SEARCH}
			DOC "SQLite3 libraries directory (contains sqlite3.lib)")
			
find_package_handle_standard_args(SQLITE3 DEFAULT_MSG SQLITE3_INCLUDE_DIR SQLITE3_LIBRARIES)

# TODO: 
# * make these work:
