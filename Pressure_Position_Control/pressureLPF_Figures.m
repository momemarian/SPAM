%% Time domain
figure 
hold on
Pressure_No_Filter.plot('r')
Pressure_PNE_LPF.plot('k')
Pressure.plot;

%% FFT
figure(1)
clf

subplot (2,1,1)
y = Pressure.data(8000:12000);
s = size(y);
L = s(1);
NFFT = 2^nextpow2(L);
Y = fft(y,NFFT)/L;

Fs = 800;
f = Fs/2*linspace(0,1,NFFT/2+1);

PY1=30+10*log(2*abs(Y(1:NFFT/2+1)));

% Plot single-sided amplitude spectrum.
plot(f,PY1) 
title('Single-Sided Amplitude Spectrum of y(t)')
xlabel('Frequency (Hz)')
ylabel('|Y(f)|')

hold on

y = Pressure_C_H_LPF.data(8000:12000);
s = size(y);
L = s(1);
NFFT = 2^nextpow2(L);
Y = fft(y,NFFT)/L;

Fs = 800;
f = Fs/2*linspace(0,1,NFFT/2+1);

PY2=30+10*log(2*abs(Y(1:NFFT/2+1)));
% Plot single-sided amplitude spectrum.
plot(f,PY2,'r') 

subplot (2,1,2)
plot(f,PY1-PY2) 
