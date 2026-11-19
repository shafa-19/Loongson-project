# loads the project binary and starts it through the EJTAG proxy
set confirm off
set architecture mips:isa32
target remote 127.0.0.2:50000
load
set $pc = start
printf "\n*** program started on the board ***\n"
continue
