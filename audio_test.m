%[y, Fs] = audioread("~/Music/john_the_fisherman.wav");
%[y, Fs] = audioread("~/Music/highway_61_revisited.wav");
[y, Fs] = audioread("~/Music/megalomania.wav");
%r = spectrogram(y(:, 1), 12e3, 200, 'yaxis');

[~,F,T,P] = spectrogram(y(:, 2), 2^12, 2^9, 2^14, 48000, 'yaxis');

figure(1);
imagesc(T, F, 10*log10(P+eps)) % add eps like pspectrogram does
axis xy
ylabel('Frequency (Hz)')
xlabel('Time (s)')
xlim([180 220]);
ylim([0 700]);
h = colorbar;
h.Label.String = 'Power/frequency (dB/Hz)';

y_band = filter(band3, 1, y(:, 2));
y_band = y_band*2;
[~,F2,T2,P2] = spectrogram(y_band, 2^12, 2^9, 2^14, 48000, 'yaxis');

figure(2);
imagesc(T2, F2, 10*log10(P2+eps)) % add eps like pspectrogram does
axis xy
ylabel('Frequency (Hz)')
xlabel('Time (s)')
xlim([180 220]);
ylim([0 700]);
h = colorbar;
h.Label.String = 'Power/frequency (dB/Hz)';
