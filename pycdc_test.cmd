::cd 
@prompt -$G

set failed=0
set tests=0
	for /r %%f in (tests\*.pyc) do @(
		@call :TestFile %%f
	)
@echo ___________________________________________
@echo.
@echo 			%failed% of %tests% tests failed.	
@goto :eof

:TestFile

	@call MSVC\Release\pyc_decomp.exe "%1" 1>nul
	@echo %errorlevel%			^<= %~nx1	
	@if not errorlevel 0 set /a failed=%failed% + 1
	
	@set /a tests=%tests% + 1
	
@goto :eof