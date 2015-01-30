
set(CBENCHSUITE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/core/benchsuite.c
	${CMAKE_CURRENT_SOURCE_DIR}/core/cbench.c
	${CMAKE_CURRENT_SOURCE_DIR}/core/data.c
	${CMAKE_CURRENT_SOURCE_DIR}/core/module_manager.c
	${CMAKE_CURRENT_SOURCE_DIR}/core/option.c
	${CMAKE_CURRENT_SOURCE_DIR}/core/plugin.c
	${CMAKE_CURRENT_SOURCE_DIR}/core/sha256.c
	${CMAKE_CURRENT_SOURCE_DIR}/core/system_info.c
	${CMAKE_CURRENT_SOURCE_DIR}/core/util.c
	${CMAKE_CURRENT_SOURCE_DIR}/core/version.c
	${CMAKE_CURRENT_SOURCE_DIR}/storage/sqlite3.c
	PARENT_SCOPE)
