Simple cron implementation

To compile:
./make

To run daemon:
./minicron taskfile outfile

taskfile format:
hour:minutes:command:info

where info must be in values:
0 - stdout enabled
1 - stderr enabled
2 - stdout and stderr enabled

Example of taskfile:
08:40:/bin/ls -l /home:2
08:39:/bin/ls -a /:2
08:30:/bin/ls aaa:2
