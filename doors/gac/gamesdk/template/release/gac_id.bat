@ECHO OFF
REM Do not modify the following area:
REM
REM This file should be used to run Tournament FreeCell. The reason is
REM that the auto update will not work if just the GAC_ID.EXE is run.
REM Check for .EXE file...
if exist IN\GAC_ID.EXE goto copy
REM
REM Run the game with any supplied parameters.
:run
GAC_ID.exe %1 %2 %3 %4 %5 %6 %7 %8 %9
goto end
REM
REM This section will copy the new .EXE to the directory (if needed)
:copy
attrib -r GAC_ID.EXE
copy IN\GAC_ID.EXE .
del IN\GAC_ID.EXE
attrib +r GAC_ID.EXE
goto run
REM
REM Exit
:end
