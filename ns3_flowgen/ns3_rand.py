#!/usr/bin/python3

import random
import math


class CdfRand:
    def __init__(self, seed=0):
        self._rand = random.Random(seed)

    def test_cdf(self, cdf):
        if cdf[0][1] != 0:
            return False
        if cdf[-1][1] != 100:
            return False
        for i in range(1, len(cdf)):
            if cdf[i][1] <= cdf[i - 1][1] or cdf[i][0] <= cdf[i - 1][0]:
                return False
        return True

    def set_cdf(self, cdf):
        if not self.test_cdf(cdf):
            return False
        self.cdf = cdf
        return True

    def get_avg(self):
        s = 0
        last_x, last_y = self.cdf[0]
        for c in self.cdf[1:]:
            x, y = c
            s += (x + last_x) / 2.0 * (y - last_y)
            last_x = x
            last_y = y
        return s / 100

    def get_rand(self):
        r = self._rand.random() * 100
        return self.get_value_from_percentile(r)

    def get_percentile_from_value(self, x):
        if x < 0 or x > self.cdf[-1][0]:
            return -1
        for i in range(1, len(self.cdf)):
            if x <= self.cdf[i][0]:
                x0, y0 = self.cdf[i - 1]
                x1, y1 = self.cdf[i]
                return y0 + (y1 - y0) / (x1 - x0) * (x - x0)

    def get_value_from_percentile(self, y):
        for i in range(1, len(self.cdf)):
            if y <= self.cdf[i][1]:
                x0, y0 = self.cdf[i - 1]
                x1, y1 = self.cdf[i]
                return x0 + (x1 - x0) / (y1 - y0) * (y - y0)

    def get_integral_y(self, y):
        s = 0
        for i in range(1, len(self.cdf)):
            x0, y0 = self.cdf[i - 1]
            x1, y1 = self.cdf[i]
            if y <= self.cdf[i][1]:
                s += (
                    0.5
                    * (x0 + x0 + (x1 - x0) / (y1 - y0) * (y - y0))
                    * (y - y0)
                    / 100.0
                )
                break
            else:
                s += 0.5 * (x1 + x0) * (y1 - y0) / 100.0
        return s


class PossionRand:
    def __init__(self, seed=0):
        self._rand = random.Random(seed)

    def get_rand(self, lam):
        return -math.log(1 - self._rand.random()) * lam
