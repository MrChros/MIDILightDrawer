@echo off
echo Starting Python script execution sequence...

py AILG_Claude.py
IF %ERRORLEVEL% NEQ 0 (
    echo Error running first_script.py
    exit /b %ERRORLEVEL%
)

py AILG_Copilot.py
IF %ERRORLEVEL% NEQ 0 (
    echo Error running second_script.py
    exit /b %ERRORLEVEL%
)

py AILG_ChatGPT.py
IF %ERRORLEVEL% NEQ 0 (
    echo Error running third_script.py
    exit /b %ERRORLEVEL%
)

echo All scripts completed successfully
pause