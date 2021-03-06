/*
                          qmusic

    Copyright (C) 2015 Arthur Benilov,
    arthur.benilov@gmail.com

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.
*/

#include <functional>
#include <qmath.h>
#include <QMap>
#include "Fft.h"

const QMap<Fft::Window, std::function<double(int, int)> > cWindows {
    { Fft::Window_None, [](int i, int n) { return 1.0; } },
    { Fft::Window_Hann, [](int i, int n) { return 0.5 * (1.0 - cos(2.0*M_PI*i/(n-1))); } },
    { Fft::Window_Hamming, [](int i, int n) { return 0.53836 + 0.46164*cos(2.0*M_PI*i/(n - 1)); } },
    { Fft::Window_Blackman, [](int i, int n) {
            double x = 2.0*M_PI*i / (n-1);
            return 0.42659 - 0.49656*cos(x) + 0.076849*cos(2.0*x);
        }
    }
};

// Source: http://rosettacode.org/wiki/Fast_Fourier_transform



// Cooley-Tukey FFT
void Fft::direct(Fft::Array &x, Window win)
{
    if (win != Window_None) {
        // Apply window
        for (int i = 0; i < x.size(); i++) {
            x[i] *= cWindows[win](i, static_cast<int>(x.size()));
        }
    }

    // DFT
    unsigned int N = (unsigned int)x.size(), k = N, n;
    double thetaT = M_PI / N;
    Complex phiT = Complex(cos(thetaT), sin(thetaT)), T;
    while (k > 1) {
        n = k;
        k >>= 1;
        phiT = phiT * phiT;
        T = 1.0L;
        for (unsigned int l = 0; l < k; l++) {
            for (unsigned int a = l; a < N; a += n) {
                unsigned int b = a + k;
                Complex t = x[a] - x[b];
                x[a] += x[b];
                x[b] = t * T;
            }
            T *= phiT;
        }
    }

    // Decimate
    unsigned int m = (unsigned int)log2(N);
    for (unsigned int a = 0; a < N; a++) {
        unsigned int b = a;
        // Reverse bits
        b = (((b & 0xaaaaaaaa) >> 1) | ((b & 0x55555555) << 1));
        b = (((b & 0xcccccccc) >> 2) | ((b & 0x33333333) << 2));
        b = (((b & 0xf0f0f0f0) >> 4) | ((b & 0x0f0f0f0f) << 4));
        b = (((b & 0xff00ff00) >> 8) | ((b & 0x00ff00ff) << 8));
        b = ((b >> 16) | (b << 16)) >> (32 - m);
        if (b > a) {
            Complex t = x[a];
            x[a] = x[b];
            x[b] = t;
        }
    }

    // Normalize
    Complex f = 1.0 / sqrt(N);
    for (unsigned int i = 0; i < N; i++) {
        x[i] *= f;
    }
}

void Fft::inverse(Array &x)
{
    // conjugate the complex numbers
    x = x.apply(std::conj);

    // forward fft
    direct(x);

    // conjugate the complex numbers again
    x = x.apply(std::conj);

    // scale the numbers
    x /= x.size();
}
