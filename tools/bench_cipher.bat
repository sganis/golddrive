@echo off
FOR /F "tokens=*" %%G IN ('ssh -Q cipher support@localhost') DO (
  echo testing %%G
  ssh -c %%G support@localhost "time -p cat > remote" < file.iso
  ssh -c %%G support@localhost  "time -p cat remote" > file-copy.iso
)