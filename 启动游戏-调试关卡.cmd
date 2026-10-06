@echo off
call "%~dp0start-game.cmd" -savedir="%~dp0dist\debugdata" -sandbox -resolution=auto -fullscreen -filter=bicubic %*
