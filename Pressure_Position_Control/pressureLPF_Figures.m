%% Time domain
figure 
hold on
Pressure.plot('r')
Angle_Y.plot('k')
Pressure_wo_LPF.plot;

%% FFT
figure(1)
clf

s = size(Gyro_Y.data);

Fs = 800;
Ts = 1/Fs;

% Test_Period = 10; % How long to run the Query Simulink
Test_Period = s(1)/Fs;

f = Fs/2*linspace(0,1,NFFT/2+1);


subplot (2,1,1)
y = Pressure_GND_R0.data(Fs*4:Fs*Test_Period);
s = size(y);
L = s(1);
NFFT = 2^nextpow2(L);
Y = fft(y,NFFT)/L;

Y1 = 2*abs(Y(1:NFFT/2+1));
PY1=30+10*log(Y1);

% Plot single-sided amplitude spectrum.
plot(f,PY1) 
title('Single-Sided Amplitude Spectrum of y(t)')
xlabel('Frequency (Hz)')
ylabel('|Y(f)|')

hold on

y = Pressure.data(Fs*4:Fs*Test_Period);
s = size(y);
L = s(1);
NFFT = 2^nextpow2(L);
Y = fft(y,NFFT)/L;

f = Fs/2*linspace(0,1,NFFT/2+1);

Y2 = 2*abs(Y(1:NFFT/2+1));
PY2=30+10*log(Y2);
% Plot single-sided amplitude spectrum.
plot(f,PY2,'r') 

subplot (2,1,2)
plot(f,PY2-PY1,'k') 
