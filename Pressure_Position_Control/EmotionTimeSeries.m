s = size(Anger_Index);
Time = (0:0.011869:(s(1)-1)*0.011869)';
AngerIndexTimeSeries = [Time Anger_Index];

s = size(Anger_Middle);
Time = (0:0.011869:(s(1)-1)*0.011869)';
AngerMiddleTimeSeries = [Time Anger_Middle];

s = size(Anger_Ring);
Time = (0:0.011869:(s(1)-1)*0.011869)';
AngerRingTimeSeries = [Time Anger_Ring];


s = size(Joy_Index);
Time = (0:0.011869:(s(1)-1)*0.011869)';
JoyIndexTimeSeries = [Time Joy_Index];

s = size(Joy_Middle);
Time = (0:0.011869:(s(1)-1)*0.011869)';
JoyMiddleTimeSeries = [Time Joy_Middle];

s = size(Joy_Ring);
Time = (0:0.011869:(s(1)-1)*0.011869)';
JoyRingTimeSeries = [Time Joy_Ring];

s = size(Sad_Index);
Time = (0:0.011869:(s(1)-1)*0.011869)';
SadIndexTimeSeries = [Time Sad_Index];

s = size(Sad_Middle);
Time = (0:0.011869:(s(1)-1)*0.011869)';
SadMiddleTimeSeries = [Time Sad_Middle];

s = size(Sad_Ring);
Time = (0:0.011869:(s(1)-1)*0.011869)';
SadRingTimeSeries = [Time Sad_Ring];
clear Time s 