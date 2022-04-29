import PyPGE as PGE
import numpy as np
import random

class BaseExample(PGE.PixelGameEngine):
    def __init__(self):
        super().__init__()
        self.sAppName = "Hello World"
        self.uQuit = False
        self.eQuit = False

    def OnUserCreate(self):
        return True

    def HandleUserInput(self):
        # Hit ESC to quit
        if self.GetKey(PGE.Key.ESCAPE).bHeld:
            self.uQuit = True

    def OnUserUpdate(self, f):
        self.HandleUserInput()
        self.Update(f)
        return not self.uQuit and not self.eQuit

    def Update(self, f):
        return

# 5-6 FPS on Ryzen 7 4800H

class Example1(BaseExample):
    def Update(self, f):
        buf = np.random.randint(0, 256, \
            size=(self.ScreenWidth(), self.ScreenHeight(), 3),
            dtype=np.uint8)
        for x in range(self.ScreenWidth()):
            for y in range(self.ScreenHeight()):
                p = PGE.Pixel(*buf[x, y])
                self.Draw(x, y, p)

# 8-9 FPS

class Example2(BaseExample):
    def Update(self, f):
        buf = np.random.randint(0, 256, \
            size=(self.ScreenWidth(), self.ScreenHeight(), 3),
            dtype=np.uint8)
        for x in range(self.ScreenWidth()):
            for y in range(self.ScreenHeight()):
                self.Draw(x, y, *buf[x, y])

# ~1300 FPS

class Example3(BaseExample):
    def Update(self, f):
        buf = np.random.randint(0, 256, \
            size=(self.ScreenHeight(), self.ScreenWidth(), 3),
            dtype=np.uint8)
        self.DrawArea(0, 0, buf)

class Example4(BaseExample):
    def Update(self, f):
        buf = random.randbytes(self.ScreenHeight() * self.ScreenWidth() * 3)
        self.DrawArea(0, 0, self.ScreenHeight(), self.ScreenWidth(), 3, self.buf)

import sys, traceback
class ExampleException(BaseExample):
    def Update(self, f):
        try:
            raise ValueError("Test")
        except:
            traceback.print_exception(*sys.exc_info())  
            self.eQuit = True          

if __name__ == "__main__":
    demo = ExampleException()
    if demo.Construct(256, 240, 2, 2):
        demo.Start()