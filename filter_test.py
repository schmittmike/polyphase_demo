#!/usr/bin/python3

from filter_coefs import bandpass_coefs
import numpy as np
from scipy.io import wavfile
import matplotlib.pyplot as plt

# M = number of taps
def polyphase_fir_2channel_wav(data, coefs, M):
    N = len(data[:,0])
    result = np.zeros((N+len(coefs), 2))

    ind = [0, 5, 4, 3, 2, 1]
    x = np.zeros([int(np.ceil((N+(M-1))/M)), M, 2])
    for ch in range(2):
        x_fill = [0, 1, 1, 1, 1, 1]
        for i in range(N):
            x[x_fill[i%M], ind[i%M], ch] = data[i, ch]
            x_fill[i%M] += 1;

    h = np.zeros([int(len(coefs)/M), M])
    for t in range(M):
        h[:, t] = coefs[t:len(coefs):M]

    for ch in range(2): # 2 channels hardcode
        for t in range(M):
            fir = np.convolve(x[:,t, ch], h[:, t])
            result[0:len(fir), ch] += fir

    return result

np.random.seed(69420)
fs, x = wavfile.read("/home/sch/Music/megalomania.wav")
print("wav shape: ", np.shape(x))
for i in range(10):
    print(x[i,:])
x = x[0:10000,:]
x = np.array(x)
N = len(x[:,0])
n = np.arange(0, N)

c = [i for i in range(6*2, 0, -1)]
taps = 1
d = polyphase_fir_2channel_wav(x, bandpass_coefs, taps)
x_filt = np.convolve(x[:,0], c)

#for i in range(len(p[:,0])):
#    print("x:", x_filt[i], " p: ", p[i, 0])
#
#for i in range(len(x_filt)):
#    print("x:", x_filt[i], " d: ", d[i, 0])


# plot bandpass filter response
#fs_2 = 24000
#fs_d = 24000*taps
#bpf = np.fft.fft(bandpass_coefs, 20000)
#fig, axs = plt.subplots(2, 1)
#axs[0].plot(np.arange(0, fs_2, fs_2/(len(bpf)/2)), 20*np.log(abs(bpf[:int(len(bpf)/2)])))
#axs[1].plot(np.arange(0, fs_d, fs_d/(len(bpf)/2)), 20*np.log(abs(bpf[:int(len(bpf)/2)])))
#for i in range(len(axs)):
#    axs[i].grid(True)
#plt.show()

for i in range(16):
    print("d[i]: ", d[i])

# save as wav file
#k = 1/(len(bandpass_coefs)*6)
#
#print("len p before: ", len(d))
#d = d[0:len(d)//taps, :]
#print("len p after: ", len(d))
#for i in range(len(d)):
#    d[i] = d[i] * k
#
#
#wavfile.write("mega_filter.wav", 48000//taps, d)
