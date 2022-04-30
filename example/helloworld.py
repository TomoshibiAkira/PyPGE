import PyPGE as PGE
import numpy as np
import random
import sys, traceback

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
        try:
            self.HandleUserInput()
            self.Update(f)
        except:
            traceback.print_exception(*sys.exc_info())  
            self.eQuit = True
        return not self.uQuit and not self.eQuit

    def Update(self, f):
        return

# CPU: Ryzen 5 3600 
# Mem: DDR4 3200MHz
# CPython v3.7.4 (conda)
# PyPy    v7.3.9 (conda-forge)

# CPython:  4-5 FPS
# PyPy:     Under 1 FPS, the code in Update loop cannot be seen by PyPy's JIT?
class ExampleStaticPythonCopy(BaseExample):
    buf = np.random.randint(0, 256, \
        size=(240, 256, 3),
        dtype=np.uint8)
    def Update(self, f):
        for x in range(self.ScreenWidth()):
            for y in range(self.ScreenHeight()):
                p = PGE.Pixel(*self.buf[x, y])
                self.Draw(x, y, p)

# CPython:  7-8 FPS
# PyPy:     Under 1 FPS, the code in Update loop cannot be seen by PyPy's JIT?
class ExampleStaticCopy(BaseExample):
    buf = np.random.randint(0, 256, \
        size=(240, 256, 3),
        dtype=np.uint8)
    def Update(self, f):
        for x in range(self.ScreenWidth()):
            for y in range(self.ScreenHeight()):
                self.Draw(x, y, *self.buf[x, y])

# CPython:  1900-2000 FPS
# PyPy:     1400-1500 FPS
class ExampleStaticNumpy(BaseExample):
    buf = np.random.randint(0, 256, \
        size=(240, 256, 3),
        dtype=np.uint8)
    def Update(self, f):
        # buf = np.random.randint(0, 256, \
        #     size=(self.ScreenHeight(), self.ScreenWidth(), 3),
        #     dtype=np.uint8)
        self.DrawArea(0, 0, self.buf)

# CPython:  1900-2000 FPS
# PyPy:     1400-1500 FPS
class ExampleStaticBytes(BaseExample):
    buf = bytes([random.randint(0, 255) for _ in range(240*256*3)])
    def Update(self, f):
        # buf = np.random.randint(0, 256, \
        #     size=(self.ScreenHeight(), self.ScreenWidth(), 3),
        #     dtype=np.uint8)
        self.DrawArea(0, 0, self.ScreenHeight(), self.ScreenWidth(), 3, self.buf)

# CPython:  1900-2000 FPS
# PyPy:     1400-1500 FPS
class ExampleStaticBytearray(BaseExample):
    buf = bytearray([random.randint(0, 255) for _ in range(240*256*3)])
    def Update(self, f):
        self.DrawArea(0, 0, self.ScreenHeight(), self.ScreenWidth(), 3, self.buf)

# CPython:  ~110 FPS
# PyPy:     ~100 FPS
class ExampleStaticNumpySprite(BaseExample):
    buf = np.random.randint(0, 256, \
        size=(240, 256, 3),
        dtype=np.uint8)
    def Update(self, f):
        self.DrawSprite(0, 0, self.buf)

# The rest of DrawSprite variants is roughly the same.
# The performance drop is easy to explain: we explicitly copied the buffer
# from our Python object to form a Sprite in PyDrawSprite (see PyPGE.cpp#L121).

# CPython:  ~110 FPS
# PyPy:     ~100 FPS
# This is not easy to explain. Since we created a Sprite object beforehand,
# there should be no copying at all... except, pybind11 did some implicit
# copying when passing the object, which is totally possible.
class ExampleStaticSpriteConstruct(BaseExample):
    def __init__(self):
        super().__init__()
        self.spr = PGE.Sprite(256, 240)
        for x in range(256):
            for y in range(240):
                self.spr.SetPixel(x, y,
                    PGE.Pixel(
                        random.randint(0, 255),
                        random.randint(0, 255),
                        random.randint(0, 255)
                    )
                )

    def Update(self, f):
        self.DrawSprite(0, 0, self.spr)



if __name__ == "__main__":
    demo = ExampleStaticSpriteConstruct()
    if demo.Construct(256, 240, 2, 2):
        demo.Start()