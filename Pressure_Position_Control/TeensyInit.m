%% Create Signals
% init_row = (200*20)+1;
% s = size(OpenLoop_DC_HSB.data(init_row:400*20)); % Anger & Sad
init_row = (50*20)+1;
s = size(OpenLoop_DC_HSB.data(init_row:250*20)); % Joy

byteOut_vector =  zeros ([s(1)*4,1]);   
DC_MAT = zeros ([1,s(1)+800]);  
REF_MAT = zeros ([1,s(1)+800]); 

row =1;

while (row <= s(1))
    byteOut_vector(row*4-3) = REF_HSB.data(row+init_row);
    byteOut_vector(row*4-2) = REF_LSB.data(row+init_row);
    byteOut_vector(row*4-1) = OpenLoop_DC_HSB.data(row+init_row);
    byteOut_vector(row*4) = OpenLoop_DC_LSB.data(row+init_row);
    DC_MAT (row)=  byteOut_vector(row*4-1)*256 + byteOut_vector(row*4);
    REF_MAT(row)=  byteOut_vector(row*4-3)*256 + byteOut_vector(row*4-2);
    row = row + 1;
end

final_diff_DC = DC_MAT (1) - DC_MAT (row-1);
DC_ending = DC_MAT (row-1)+final_diff_DC/800:final_diff_DC/800:DC_MAT (1);

final_diff_ref = REF_MAT (1) - REF_MAT (row-1);
REF_ending = REF_MAT (row-1)+final_diff_ref/800:final_diff_ref/800:REF_MAT (1);

while (row <= s(1) +800)
    if (final_diff_DC ~= 0)
        tmp = round(DC_ending(row-s(1)));
    else
        tmp = 0;
    end
    byteOut_vector(row*4-1) = floor(tmp/256);
    byteOut_vector(row*4) = tmp - byteOut_vector(row*4-1)*256;
    DC_MAT (row)=  byteOut_vector(row*4-1)*256 +byteOut_vector(row*4) ;
    
    if (final_diff_ref ~= 0) 
        tmp = round(REF_ending(row-s(1)));
    else
        tmp = 0;
    end
    byteOut_vector(row*4-3) = floor(tmp/256);
    byteOut_vector(row*4-2) = tmp - byteOut_vector(row*4-3)*256;
    REF_MAT (row)=  byteOut_vector(row*4-3)*256 +byteOut_vector(row*4-2) ;
     
    row = row + 1;
end

tmpTime = 1/800:1/800:1+s(1)/800;


% figure(1)
% plot (tmpTime,DC_MAT )
% figure(2)
% plot (tmpTime,REF_MAT,'r' )

clear tmpTime DC_ending row DC_MAT REF_MAT tmp final_diff_ref final_diff_DC s init_row tmpTime
%% Transmit Signals

Fs = 800;
Ts = 1/Fs;

system start_teensy_gateway.bat
teensyConnection= tcpip('127.0.0.1',28541,'Terminator','y');
fopen(teensyConnection);

fwrite(teensyConnection,'G');
row =1; 
s = size(byteOut_vector);

while (row < s(1))
    fprintf (teensyConnection,'%uy%uy%uy%uy%uy%uy%uy%uy%uy%uy%uy%uy%uy%uy%uy%uy%uy%uy%uy%uy',byteOut_vector(row:row+19));
    row = row + 20;
end
% Terminate byte send
fwrite(teensyConnection,'Ty');

%Wait for the Teensy to confirm

fscanf(teensyConnection,'Ty');
fscanf(teensyConnection,'%sy');
while(teensyConnection.BytesAvailable ==0 )
end
fscanf(teensyConnection,'%cy');

%Start Teensy Loop
fwrite(teensyConnection,'G');
fclose(teensyConnection);


clear row s

% teensyConnection= tcpip('127.0.0.1',28541,'Terminator','y');
% fopen(teensyConnection); 
% fwrite(teensyConnection,'G');
% fclose(teensyConnection);