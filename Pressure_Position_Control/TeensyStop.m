fopen(teensyConnection);
fwrite(teensyConnection,'S');
fclose(teensyConnection);
cuur_time = clock;
save_name = ['../../SPAMBackupData/' num2str(cuur_time(1)) '_' num2str(cuur_time(2)) '_' ... 
    num2str(cuur_time(3)) '_' num2str(cuur_time(4)) '_'...
    num2str(cuur_time(5)) '_' num2str(round(cuur_time(6)))];

save(save_name);
system stop_teensy_gateway.bat