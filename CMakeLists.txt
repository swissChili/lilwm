cmake_minimum_required(VERSION 3.13)

project(lilwm C)

include_directories(fastkv lilui)
subdirs(fastkv)

add_library(lilui
	lilui/lilui.h
	lilui/lilui.c
	lilui/widgets.c)
target_link_libraries(lilui X11)

add_executable(testlilui
	test.c)
target_link_libraries(testlilui lilui)

add_executable(msgbox
	msgbox/msgbox.c)
target_link_libraries(msgbox lilui)

add_executable(lilwm
	lilwm.c config.c config.h)
target_link_libraries(lilwm libfastkv X11)

install(TARGETS lilwm lilui msgbox
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib/static)
