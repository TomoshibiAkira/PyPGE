import PyPGE as PGE
import numpy as np

class Breakout(PGE.PixelGameEngine):
    COLLISION_DETECT_POINT = [
        PGE.vf2d(0, -1),
        PGE.vf2d(0, 1),
        PGE.vf2d(-1, 0),
        PGE.vf2d(1, 0)
    ]

    def __init__(self):
        super().__init__()
        self.sAppName = "Breakout"
        self.uQuit = False
        self.fBatPos = 20.0
        self.fBatWidth = 40.0
        self.fBatSpeed = 250.0

        self.fBallRadius = 5.0
        self.fBallSpeed = 20.0
        self.vBallPos = PGE.vf2d(0.0, 0.0)
        self.vBallDir = PGE.vf2d(0.0, 0.0)

        self.vBall = PGE.vf2d(200.0, 200.0)
        self.vBlockSize = PGE.vi2d(16, 16)

    def OnUserCreate(self):
        self.sprMap = np.zeros((24, 30), dtype=np.uint32)
        self.sprMap[0] = 12
        self.sprMap[:, 0] = 12
        self.sprMap[-1] = 12
        self.sprMap[3:21, 4:6] = 1
        self.sprMap[3:21, 6:8] = 2
        self.sprMap[3:21, 8:10] = 3
        self.sprTile = PGE.Sprite("./tut_tiles.png")

        angle = np.random.random() * 2.0 * np.pi
        self.vBallDir.x = np.cos(angle)
        self.vBallDir.y = np.sin(angle)
        self.vBallPos.x = 12.5
        self.vBallPos.y = 15.5
        return True

    def HandleUserInput(self, fElapsedTime):
        # Hit ESC to quit
        if self.GetKey(PGE.Key.ESCAPE).bHeld:
            self.uQuit = True
        
        if self.GetKey(PGE.Key.LEFT).bHeld:
            self.fBatPos -= self.fBatSpeed * fElapsedTime
        if self.GetKey(PGE.Key.RIGHT).bHeld:
            self.fBatPos += self.fBatSpeed * fElapsedTime

        if self.fBatPos < 11.0:
            self.fBatPos = 11.0
        if self.fBatPos + self.fBatWidth > float(self.ScreenWidth()) - 10.0:
            self.fBatPos = float(self.ScreenWidth()) - 10.0 - self.fBatWidth
        
        if self.GetMouse(0).bHeld:
            self.vBall.x = float(self.GetMouseX())
            self.vBall.y = float(self.GetMouseY())

        if self.GetMouseWheel() > 0:
            self.fBallRadius += 1.0
        if self.GetMouseWheel() < 0:
            self.fBallRadius -= 1.0
        if self.fBallRadius < 5.0:
            self.fBallRadius = 5.0

    def OnUserUpdate(self, fElapsedTime):
        self.HandleUserInput(fElapsedTime)
        self.CollisionDetect(fElapsedTime)
        self.Update(fElapsedTime)
        return not self.uQuit

    def CollisionDetect(self, fElapsedTime):
        vPotentialBallPos = \
            self.vBallPos + self.vBallDir * self.fBallSpeed * fElapsedTime
        vTileBallRadialDims = PGE.vf2d(self.fBallRadius / self.vBlockSize.x, self.fBallRadius / self.vBlockSize.y)

        def TestResolveCollisionPoint(point):
            _t = vPotentialBallPos + vTileBallRadialDims * point
            vTestPoint = PGE.vi2d(_t)
            tile = self.sprMap[vTestPoint.x, vTestPoint.y]
            if tile == 0:
                return False
            else:
                bTileHit = tile < 10
                if (bTileHit):
                    self.sprMap[vTestPoint.x, vTestPoint.y] -= 1
                if (point.x == 0):
                    self.vBallDir.y *= -1.0
                if (point.y == 0):
                    self.vBallDir.x *= -1.0
                return True
            
        bHasHitTile = False
        for i in range(4):
            bHasHitTile |= TestResolveCollisionPoint(self.COLLISION_DETECT_POINT[i])

        if (self.vBallPos.y > 20.0):
            self.vBallDir.y *= -1.0
        
    # 100+ FPS on Ryzen 7 4800H
    def Update(self, fElapsedTime):
        self.vBallPos += self.vBallDir * self.fBallSpeed * fElapsedTime
        self.Clear(PGE.DARK_BLUE)
        self.SetPixelMode(PGE.Pixel.MASK)
        for y in range(self.sprMap.shape[1]):
            for x in range(self.sprMap.shape[0]):
                if self.sprMap[x, y] > 0:
                    self.DrawPartialSprite(\
                        x * self.vBlockSize.x, y * self.vBlockSize.y,
                        self.sprTile,
                        (self.sprMap[x, y] % 4) * self.vBlockSize.x, 0,
                        self.vBlockSize.x, self.vBlockSize.y)
        self.SetPixelMode(PGE.Pixel.NORMAL)

        # Here we have to convert everything to int explicitly, otherwise the program will crash.
        self.FillCircle(\
            int(self.vBallPos.x * self.vBlockSize.x), int(self.vBallPos.y * self.vBlockSize.y),
            int(self.fBallRadius), PGE.CYAN)
        

if __name__ == "__main__":
    demo = Breakout()
    if demo.Construct(512, 480, 2, 2):
        demo.Start()