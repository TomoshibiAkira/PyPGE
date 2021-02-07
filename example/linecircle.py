import PyPGE as PGE
import numpy as np

# 2000+ FPS on Ryzen 7 4800H

class Example(PGE.PixelGameEngine):
    def __init__(self):
        super().__init__()
        self.sAppName = "Line Circle"

    def OnUserCreate(self):
        return True

    # Since this is called in an event loop and not in the main thread,
    # any exception threw here will not be catched, instead the program
    # will crash.
    def OnUserUpdate(self, f):
        flag = np.random.random() > 0.5
        buf = np.random.randint(0, 256, size=3)
        p = PGE.Pixel(*buf)
        if flag:
            x1 = np.random.randint(0, self.ScreenWidth())
            x2 = np.random.randint(0, self.ScreenWidth())
            y1 = np.random.randint(0, self.ScreenHeight())
            y2 = np.random.randint(0, self.ScreenHeight())
            self.DrawLine(x1, y1, x2, y2, p)
        else:
            x = np.random.randint(0, self.ScreenWidth())
            y = np.random.randint(0, self.ScreenHeight())
            radius = min(min(x, self.ScreenWidth()-x),
                         min(y, self.ScreenHeight()-y))
            if radius > 1:
                radius = np.random.randint(1, radius)
            self.FillCircle(x, y, radius, p)
        return not self.GetKey(PGE.Key.ESCAPE).bHeld

if __name__ == "__main__":
    demo = Example()
    if demo.Construct(256, 240, 2, 2):
        demo.Start()