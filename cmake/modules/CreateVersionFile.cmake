find_package ( Git )

execute_process ( COMMAND git describe --abbrev=4 HEAD
                  COMMAND sed -e "s/-/./g"
                  OUTPUT_VARIABLE GP_GIT_VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE )


string ( REGEX REPLACE "^([0-9]+)\\..*" "\\1" GP_VERSION_MAJOR "${GP_GIT_VERSION}" )
string ( REGEX REPLACE "^[0-9]+\\.([0-9]+).*" "\\1" GP_VERSION_MINOR "${GP_GIT_VERSION}" )
string ( REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" GP_VERSION_PATCH "${GP_GIT_VERSION}" )
string ( REGEX REPLACE "^[0-9]+\\.[0-9]+\\.[0-9]+.(.*)" "\\1" GP_VERSION_SHA1 "${GP_GIT_VERSION}" )

set ( GP_VERSION_SHORT "${GP_VERSION_MAJOR}.${GP_VERSION_MINOR}.${GP_VERSION_PATCH}" )

message ( STATUS "${GP_VERSION_MAJOR}" )
message ( STATUS "${GP_VERSION_MINOR}" )
message ( STATUS "${GP_VERSION_PATCH}" )
message ( STATUS "${GP_VERSION_SHA1}" )
message ( STATUS "${GP_VERSION_SHORT}" )