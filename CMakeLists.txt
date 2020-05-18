cmake_minimum_required(VERSION 3.13)

project(lilwm C)

include_directories(/usr/include/freetype2)
find_package(X11 REQUIRED)

include_directories(fastkv lilui)
subdirs(fastkv)

add_library(lilui
	lilui/lilui.h
	lilui/lilui.c
	lilui/widgets.c)
target_link_libraries(lilui X11 Xft libfastkv)

add_executable(testlilui
	test.c)
target_link_libraries(testlilui lilui)

add_executable(msgbox
	msgbox/msgbox.c)
target_link_libraries(msgbox lilui)

add_executable(listpicker
	listpicker/listpicker.c listpicker/levenshtein.c)
target_link_libraries(listpicker lilui)

add_executable(lilwm
	lilwm/lilwm.c lilwm/config.c lilwm/config.h lilwm/tagged.h lilwm/tagged.c
	lilwm/keys.h lilwm/keys.c)
target_link_libraries(lilwm libfastkv X11 Xft)

install(TARGETS lilwm lilui msgbox listpicker
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib/static)

install(FILES lilwm.desktop
	DESTINATION /usr/share/xsessions
	PERMISSIONS WORLD_READ)

install(FILES bin/rundesktop bin/pickdesktop
	DESTINATION bin
	PERMISSIONS WORLD_EXECUTE WORLD_READ)

# install(FILES lilwmrc
#	DESTINATION /etc/lilwm/
#	PERMISSIONS world_read)

install(FILES lilui/basictheme.res
	DESTINATION /etc/lilui/
	PERMISSIONS WORLD_READ)
