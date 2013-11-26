def drift(freq,ppm,interval):
    a = freq*ppm
    b = a/(freq*1000.0)
    c = b * interval
    d = 1000.0 / 24.0
    return a, b, c, d