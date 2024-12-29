@echo Off

echo. 
echo -----------------------
echo Updating Build Number...
echo -----------------------

set /p version=<build_version.txt
@echo Major version number: %version%
set /p old=< build_number.txt
set /a new=%old%+1
@echo New build number: %new%
@echo Software Version: %version%.%new%
echo. 
@echo %new% > build_number.txt

if %new% lss 10 						set version_build_string=%version%.000%new%
if %new% geq 10 	if %new% lss 100 	set version_build_string=%version%.00%new%
if %new% geq 100	if %new% lss 1000 	set version_build_string=%version%.0%new%
if %new% geq 1000 						set version_build_string=%version%.%new%

(
	echo #define VERSION %version%
	echo #define BUILD %new%
	echo #define VERSION_BUILD_STRING "%version_build_string%"
) > Version.h