@echo off
IF EXIST "D:\\Programmi\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    call "D:\\Programmi\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
) ELSE (
    call "D:\\Programmi\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
)
set compilerflags=/Od /Zi /EHsc /MT
set includedirs=/I../include
set linkerflags=/LIBPATH:../libs/win glfw3.lib assimp-vc142-mt.lib zlib.lib IrrXML.lib gdi32.lib user32.lib Shell32.lib Bullet3Common.lib BulletCollision.lib BulletDynamics.lib LinearMath.lib
cl.exe %compilerflags% %includedirs% ../include/glad/glad.c main.cpp /Fe:main.exe /link %linkerflags% 
