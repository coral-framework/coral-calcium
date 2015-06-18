@echo on

mkdir build
chdir build

mkdir vc10_x64
chdir vc10_x64

del CMakeCache.txt

cmake -G "Visual Studio 10 Win64" -DMANUAL_TESTING=0 -DCMAKE_INSTALL_PREFIX="%CORAL_ROOT%" ../../

if %errorlevel% NEQ 0 goto error
goto end

:error
echo Houve um erro. Pressione qualquer tecla para finalizar.
pause >nul

:end