rem Copy libfatfs archive to DMASDSPI Component's library folder.
cd /d %~dp0
copy /Y .\libfatfs.cylib\CortexM3\ARM_GCC_493\Debug\libfatfs.a .\DMASDSPITest.cydsn\DMASDSPI\Library\libfatfsd.a
copy /Y .\libfatfs.cylib\CortexM3\ARM_GCC_493\Release\libfatfs.a .\DMASDSPITest.cydsn\DMASDSPI\Library\libfatfs.a
