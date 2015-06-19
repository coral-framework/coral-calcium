@echo on

cd..

mkdir build
chdir build

mkdir vc12_x64
chdir vc12_x64

del CMakeCache.txt

cmake -G "Visual Studio 12 2013 Win64" -DMANUAL_TESTING=0 -DCMAKE_INSTALL_PREFIX="%CORAL_ROOT%" ../../

if %errorlevel% NEQ 0 goto error
goto end

:error
echo Houve um erro. Pressione qualquer tecla para finalizar.
pause >nul

:end