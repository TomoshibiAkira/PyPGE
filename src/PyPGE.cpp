#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include <pybind11/numpy.h>

#define OLC_IMAGE_STB 
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

/* Inherit the original class to add more convenient functions for Python. */
// Experimental
class PGE : public olc::PixelGameEngine
{
public:
    using olc::PixelGameEngine::PixelGameEngine;

    bool PyDraw(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = olc::nDefaultAlpha)
    {
        olc::Pixel p(r, g, b, a);
        return olc::PixelGameEngine::Draw(x, y, p);
    }

    bool PyDrawArea(int32_t x, int32_t y, int32_t ah, int32_t aw, int32_t c, const py::bytes& data, int32_t ox, int32_t oy, int32_t w, int32_t h, int32_t scale, int32_t flip)
    {
        _drawArea(x, y, ah, aw, c, (const uint8_t *)std::string_view(data).data(), ox, oy, w, h, scale, flip);
        return true;
    }

    bool PyDrawArea(int32_t x, int32_t y, int32_t ah, int32_t aw, int32_t c, const py::bytearray& data, int32_t ox, int32_t oy, int32_t w, int32_t h, int32_t scale, int32_t flip)
    {
        _drawArea(x, y, ah, aw, c, (const uint8_t *)std::string(data).data(), ox, oy, w, h, scale, flip);
        return true;
    }

    void _drawArea(int32_t x, int32_t y, 
        int32_t ah, int32_t aw, int32_t c, const uint8_t* data,
        int32_t ox, int32_t oy, int32_t w, int32_t h,
        int32_t scale, int32_t flip)
    {
        if (c == 3)
        {
            for (uint32_t i = oy; i < oy + h; i++)
                for (uint32_t si = 0; si < scale; si++) {
                    uint32_t py = (flip & olc::Sprite::Flip::VERT) ? ah - i - 1 : i;
                    py = y + (py - oy) * scale + si;
                    for (uint32_t j = ox; j < ox + w; j++) {
                        uint32_t pos = i * aw + j;
                        for (uint32_t sj = 0; sj < scale; sj++)
                        {
                            uint32_t px = (flip & olc::Sprite::Flip::HORIZ) ? aw - j - 1 : j;
                            px = x + (px - ox) * scale + sj;
                            olc::PixelGameEngine::Draw(px, py,
                                olc::Pixel(
                                    data[3 * pos],
                                    data[3 * pos+1],
                                    data[3 * pos+2]
                                ));
                        }
                    }
                }
        }
        else if (c == 4)
        {
            for (uint32_t i = oy; i < oy + h; i++)
                for (uint32_t si = 0; si < scale; si++) {
                    uint32_t py = (flip & olc::Sprite::Flip::VERT) ? ah - i - 1 : i;
                    py = y + (py - oy) * scale + si;
                    for (uint32_t j = ox; j < ox + w; j++) {
                        uint32_t pos = i * aw + j;
                        for (uint32_t sj = 0; sj < scale; sj++)
                        {
                            uint32_t px = (flip & olc::Sprite::Flip::HORIZ) ? aw - j - 1 : j;
                            px = x + (px - ox) * scale + sj;
                            olc::PixelGameEngine::Draw(px, py,
                                olc::Pixel(
                                    data[4 * pos],
                                    data[4 * pos+1],
                                    data[4 * pos+2],
                                    data[4 * pos+3]
                                ));
                        }
                    }
                }
        }
        else
            throw std::runtime_error ("only support 3-channel RGB or 4-channel RGBA");
    }

    bool PyDrawArea(int32_t x, int32_t y, int32_t ah, int32_t aw, int32_t c, const py::bytearray& data, int32_t scale, int32_t flip)
    {
        // pybind11::bytearray does not support std::string_view yet in 2.9.2. This feature has already been implemented and merged,
        // but didn't make to 2.9.2's release (https://github.com/pybind/pybind11/pull/3707). 2.9.3 maybe?
        // As for now, we have to make a copy for the bytearray... oh well.
        _drawArea(x, y, ah, aw, c, (const uint8_t *)std::string(data).data(), 0, 0, aw, ah, scale, flip);
        return true;
    }

    bool PyDrawArea(int32_t x, int32_t y, int32_t ah, int32_t aw, int32_t c, const py::bytes& data, int32_t scale, int32_t flip)
    {
        _drawArea(x, y, ah, aw, c, (const uint8_t *)std::string_view(data).data(), 0, 0, aw, ah, scale, flip);
        return true;
    }

    bool PyDrawArea(int32_t x, int32_t y, const py::array_t<uint8_t>& pm, int32_t scale, int32_t flip)
    {
        py::buffer_info info = pm.request();

        uint8_t* data = (uint8_t*)info.ptr;
        int32_t aw = (int32_t)info.shape[1];
        int32_t ah = (int32_t)info.shape[0];
        int32_t c = (int32_t)info.shape[2];

        if (info.ndim != 3)
            throw std::runtime_error ("ndim should be 3");

        _drawArea(x, y, ah, aw, c, data, 0, 0, aw, ah, scale, flip);
        return true;
    }

    /* -----------------------------------------------------------------------------------------
    DrawSprite without actually construct the sprite in Python. The main reason that the
    performance drops is probably the extra buffer copying.
    Since olc::DrawSprite also use olc::Draw, just use DrawArea instead. It is completely
    compatible with DrawSprite (without constructing the Sprite object ofc).
    There is a way to do this without copying if uint32_t and olc::Pixel can be easily converted.
    In Python code one can construct a list of int (32bit) to represent RGBA, exactly like 
    olc::Sprite's underlying buffer (pColData). Once the list is converted into vector<int> by
    pybind11, here we can use an extension of olc::Sprite and take the vector as the underlying
    buffer.
    But for now, it seems olc::Pixel is its own thing...
    -------------------------------------------------------------------------------------------- */
    void _pyDrawSprite(int32_t x, int32_t y, int32_t ah, int32_t aw, int32_t c, const uint8_t* data, uint32_t scale, uint8_t flip)
    {
        olc::Sprite spr(aw, ah);
        if (c == 3)
        {
            for (uint32_t i = 0; i < ah; i++)
                for (uint32_t j = 0; j < aw; j++)
                {
                    uint32_t pos = i * ah + j;
                    spr.SetPixel(j, i,
                        olc::Pixel(
                            data[3 * pos],
                            data[3 * pos+1],
                            data[3 * pos+2]
                        ));
                }
        }
        else if (c == 4)
        {
            for (uint32_t i = 0; i < ah; i++)
                for (uint32_t j = 0; j < aw; j++)
                {
                    uint32_t pos = i * ah + j;
                    spr.SetPixel(j, i,
                        olc::Pixel(
                            data[4 * pos],
                            data[4 * pos+1],
                            data[4 * pos+2],
                            data[4 * pos+3]
                        ));
                }
        }
        else
            throw std::runtime_error ("only support 3-channel RGB or 4-channel RGBA");
        DrawSprite(x, y, &spr, scale, flip);
    }

    bool PyDrawSprite(int32_t x, int32_t y, int32_t ah, int32_t aw, int32_t c, const py::bytes& data, int32_t scale, int32_t flip)
    {
        _pyDrawSprite(x, y, ah, aw, c, (const uint8_t *)std::string_view(data).data(), scale, flip);
        return true;
    }

    bool PyDrawSprite(int32_t x, int32_t y, int32_t ah, int32_t aw, int32_t c, const py::bytearray& data, int32_t scale, int32_t flip)
    {
        _pyDrawSprite(x, y, ah, aw, c, (const uint8_t *)std::string(data).data(), scale, flip);
        return true;
    }

    bool PyDrawSprite(int32_t x, int32_t y, const py::array_t<uint8_t>& pm, int32_t scale, int32_t flip)
    {
        py::buffer_info info = pm.request();

        uint8_t* data = (uint8_t*)info.ptr;
        int32_t aw = (int32_t)info.shape[1];
        int32_t ah = (int32_t)info.shape[0];
        int32_t c = (int32_t)info.shape[2];

        if (info.ndim != 3)
            throw std::runtime_error ("ndim should be 3");

        _pyDrawSprite(x, y, ah, aw, c, data, scale, flip);
        return true;
    }
};

/* Pybind wrapper needed for virtual functions. */
class PyPGE : public PGE
{
public:
    // Constructor
    using PGE::PGE;
    
    // User-override virtual functions
    bool OnUserCreate() override
    {
        py::gil_scoped_acquire acquire;
        PYBIND11_OVERRIDE(
            bool,                   // Return type
            PGE,   // Parent Class
            OnUserCreate
            ,
        );
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        py::gil_scoped_acquire acquire;
        PYBIND11_OVERRIDE(
            bool,                   // Return type
            PGE,   // Parent Class
            OnUserUpdate,
            fElapsedTime
        );
    }

    bool OnUserDestroy() override
    {
        py::gil_scoped_acquire acquire;
        PYBIND11_OVERRIDE(
            bool,                   // Return type
            PGE,   // Parent Class
            OnUserDestroy
            ,
        );
    }

    bool Draw(int32_t x, int32_t y, olc::Pixel p) override
    {
        py::gil_scoped_acquire acquire;
        PYBIND11_OVERRIDE(
            bool,
            PGE,
            Draw,
            x,
            y,
            p
        );
    }
};

void def_PGE(py::module& m)
{
    py::class_<PGE, PyPGE> PGE(m, "PixelGameEngine");
    PGE.def(py::init<>())
       .def("OnUserCreate", &PGE::OnUserCreate,
            "Called once on application startup, use to load your resources")
       .def("OnUserUpdate", &PGE::OnUserUpdate,
            "Called every frame, and provides you with a time per frame value")
       .def("OnUserDestroy", &PGE::OnUserDestroy,
            "Called once on application termination, so you can be one clean coder")
       .def("Draw", py::overload_cast<int32_t, int32_t, olc::Pixel>(&PGE::Draw),
            "Draws a single Pixel",
            py::arg("x"),
            py::arg("y"),
            py::arg("p") = olc::WHITE)
       .def("Draw", py::overload_cast<const olc::vi2d&, olc::Pixel>(&PGE::Draw),
            "Draws a single Pixel",
            py::arg("pos"),
            py::arg("p") = olc::WHITE)
        .def("Draw", &PyPGE::PyDraw,
            "Draws a single Pixel (Experimental)",
            py::arg("x"),
            py::arg("y"),
            py::arg("r"),
            py::arg("g"),
            py::arg("b"),
            py::arg("a") = olc::nDefaultAlpha)
        .def("DrawArea", py::overload_cast<int32_t, int32_t, int32_t, int32_t, int32_t, const py::bytes&, \
                                            int32_t, int32_t, int32_t, int32_t, int32_t, int32_t>(&PyPGE::PyDrawArea),
            "Draws an sprite with a bytes-based buffer but faster (partial)",
            py::arg("x"),
            py::arg("y"),
            py::arg("h"),
            py::arg("w"),
            py::arg("c"),
            py::arg("data"),
            py::arg("ox"),
            py::arg("oy"),
            py::arg("w"),
            py::arg("h"),
            py::arg("scale") = 1,
            py::arg("flip") = olc::Sprite::Flip::NONE)
        .def("DrawArea", py::overload_cast<int32_t, int32_t, int32_t, int32_t, int32_t, const py::bytearray&, \
                                            int32_t, int32_t, int32_t, int32_t, int32_t, int32_t>(&PyPGE::PyDrawArea),
            "Draws an sprite with a bytes-based buffer but faster (partial)",
            py::arg("x"),
            py::arg("y"),
            py::arg("h"),
            py::arg("w"),
            py::arg("c"),
            py::arg("data"),
            py::arg("ox"),
            py::arg("oy"),
            py::arg("w"),
            py::arg("h"),
            py::arg("scale") = 1,
            py::arg("flip") = olc::Sprite::Flip::NONE)
        .def("DrawArea", py::overload_cast<int32_t, int32_t, int32_t, int32_t, int32_t, const py::bytes&, int32_t, int32_t>(&
        PyPGE::PyDrawArea),
            "Draws an sprite with a bytes-based buffer but faster",
            py::arg("x"),
            py::arg("y"),
            py::arg("h"),
            py::arg("w"),
            py::arg("c"),
            py::arg("data"),
            py::arg("scale") = 1,
            py::arg("flip") = olc::Sprite::Flip::NONE)
        .def("DrawArea", py::overload_cast<int32_t, int32_t, int32_t, int32_t, int32_t, const py::bytearray&, int32_t, int32_t>(&PyPGE::PyDrawArea),
            "Draws an sprite with a bytearray-based buffer but faster",
            py::arg("x"),
            py::arg("y"),
            py::arg("h"),
            py::arg("w"),
            py::arg("c"),
            py::arg("data"),
            py::arg("scale") = 1,
            py::arg("flip") = olc::Sprite::Flip::NONE)
        .def("DrawArea", py::overload_cast<int32_t, int32_t, const py::array_t<uint8_t>&, int32_t, int32_t>(&PyPGE::PyDrawArea),
            "Draws an sprite with a numpy-based buffer but faster",
            py::arg("x"),
            py::arg("y"),
            py::arg("pm"),
            py::arg("scale") = 1,
            py::arg("flip") = olc::Sprite::Flip::NONE)
        .def("DrawSprite", py::overload_cast<int32_t, int32_t, int32_t, int32_t, int32_t, const py::bytes&, int32_t, int32_t>(&
        PyPGE::PyDrawSprite),
            "Draws an sprite with a bytes-based buffer",
            py::arg("x"),
            py::arg("y"),
            py::arg("h"),
            py::arg("w"),
            py::arg("c"),
            py::arg("data"),
            py::arg("scale") = 1,
            py::arg("flip") = olc::Sprite::Flip::NONE)
        .def("DrawSprite", py::overload_cast<int32_t, int32_t, int32_t, int32_t, int32_t, const py::bytearray&, int32_t, int32_t>(&PyPGE::PyDrawSprite),
            "Draws an sprite with a bytearray-based buffer",
            py::arg("x"),
            py::arg("y"),
            py::arg("h"),
            py::arg("w"),
            py::arg("c"),
            py::arg("data"),
            py::arg("scale") = 1,
            py::arg("flip") = olc::Sprite::Flip::NONE)
        .def("DrawSprite", py::overload_cast<int32_t, int32_t, const py::array_t<uint8_t>&, int32_t, int32_t>(&PyPGE::PyDrawSprite),
            "Draws an sprite with a numpy-based buffer",
            py::arg("x"),
            py::arg("y"),
            py::arg("pm"),
            py::arg("scale") = 1,
            py::arg("flip") = olc::Sprite::Flip::NONE);


    PGE.def_readwrite("sAppName", &PGE::sAppName);
    PGE.def("Construct", &PGE::Construct,
            py::arg("screen_w"),
            py::arg("screen_h"),
            py::arg("pixel_w"),
            py::arg("pixel_h"),
            py::arg("full_screen") = false,
            py::arg("vsync") = false,
            py::arg("cohesion") = false);
    PGE.def("Start", &PGE::Start,
            py::call_guard<py::gil_scoped_release>());

    PGE.def("IsFocused", &PGE::IsFocused,
            "Returns true if window is currently in focus\n")
        .def("GetKey", &PGE::GetKey,
            "Get the state of a specific keyboard button\n",
            py::arg("k"))
        .def("GetMouse", &PGE::GetMouse,
            "Get the state of a specific mouse button\n",
            py::arg("b"))
        .def("GetMouseX", &PGE::GetMouseX,
            "Get Mouse X coordinate in \"pixel\" space\n")
        .def("GetMouseY", &PGE::GetMouseY,
            "Get Mouse Y coordinate in \"pixel\" space\n")
        .def("GetMouseWheel", &PGE::GetMouseWheel,
            "Get Mouse Wheel Delta\n")
        .def("GetWindowMouse", &PGE::GetWindowMouse,
            "Get the mouse in window space\n")
        .def("GetMousePos", &PGE::GetMousePos,
            "Gets the mouse as a vector to keep Tarriest happy\n")
        .def("ScreenWidth", &PGE::ScreenWidth,
            "Returns the width of the screen in \"pixels\"\n")
        .def("ScreenHeight", &PGE::ScreenHeight,
            "Returns the height of the screen in \"pixels\"\n")
        .def("GetDrawTargetWidth", &PGE::GetDrawTargetWidth,
            "Returns the width of the currently selected drawing target in \"pixels\"\n")
        .def("GetDrawTargetHeight", &PGE::GetDrawTargetHeight,
            "Returns the height of the currently selected drawing target in \"pixels\"\n")
        .def("GetDrawTarget", &PGE::GetDrawTarget,
            "Returns the currently active draw target\n")
        .def("SetScreenSize", &PGE::SetScreenSize,
            "Resize the primary screen sprite\n",
            py::arg("w"),
            py::arg("h"))
        .def("SetDrawTarget", py::overload_cast<olc::Sprite*>(&PGE::SetDrawTarget),
            "Specify which Sprite should be the target of drawing functions, use nullptr\nto specify the primary screen\n",
            py::arg("target"))
        .def("SetDrawTarget", py::overload_cast<uint8_t>(&PGE::SetDrawTarget),
            "Specify which Sprite should be the target of drawing functions, use nullptr\nto specify the primary screen\n",
            py::arg("layer"))
        .def("GetFPS", &PGE::GetFPS,
            "Gets the current Frames Per Second\n")
        .def("GetElapsedTime", &PGE::GetElapsedTime,
            "Gets last update of elapsed time\n")
        .def("GetWindowSize", &PGE::GetWindowSize,
            "Gets Actual Window size\n")
        .def("GetPixelSize", &PGE::GetPixelSize,
            "Gets pixel scale\n")
        .def("GetScreenPixelSize", &PGE::GetScreenPixelSize,
            "Gets actual pixel scale\n")
        .def("EnableLayer", &PGE::EnableLayer,
            py::arg("layer"),
            py::arg("b"))
        .def("SetLayerOffset", py::overload_cast<uint8_t, const olc::vf2d&>(&PGE::SetLayerOffset),
            py::arg("layer"),
            py::arg("offset"))
        .def("SetLayerOffset", py::overload_cast<uint8_t, float, float>(&PGE::SetLayerOffset),
            py::arg("layer"),
            py::arg("x"),
            py::arg("y"))
        .def("SetLayerScale", py::overload_cast<uint8_t, const olc::vf2d&>(&PGE::SetLayerScale),
            py::arg("layer"),
            py::arg("scale"))
        .def("SetLayerScale", py::overload_cast<uint8_t, float, float>(&PGE::SetLayerScale),
            py::arg("layer"),
            py::arg("x"),
            py::arg("y"))
        .def("SetLayerTint", &PGE::SetLayerTint,
            py::arg("layer"),
            py::arg("tint"))
        .def("SetLayerCustomRenderFunction", &PGE::SetLayerCustomRenderFunction,
            py::arg("layer"),
            py::arg("f"))
        .def("GetLayers", &PGE::GetLayers)
        .def("CreateLayer", &PGE::CreateLayer)
        .def("SetPixelMode", py::overload_cast<olc::Pixel::Mode>(&PGE::SetPixelMode),
            "Change the pixel mode for different optimisations\nolc::Pixel::NORMAL = No transparency\nolc::Pixel::MASK   = Transparent if alpha is < 255\nolc::Pixel::ALPHA  = Full transparency\n",
            py::arg("m"))
        .def("SetPixelMode", py::overload_cast<std::function<olc::Pixel(const int x, const int y, const olc::Pixel& pSource, const olc::Pixel& pDest)>>(&PGE::SetPixelMode),
            "Change the pixel mode for different optimisations\nolc::Pixel::NORMAL = No transparency\nolc::Pixel::MASK   = Transparent if alpha is < 255\nolc::Pixel::ALPHA  = Full transparency\n",
            py::arg("pixelMode"))
        .def("GetPixelMode", &PGE::GetPixelMode)
        .def("SetPixelBlend", &PGE::SetPixelBlend,
            "Change the blend factor from between 0.0f to 1.0f;\n",
            py::arg("fBlend"))
        .def("DrawLine", py::overload_cast<int32_t, int32_t, int32_t, int32_t, olc::Pixel, uint32_t>(&PGE::DrawLine),
            "Draws a line from (x1,y1) to (x2,y2)\n",
            py::arg("x1"),
            py::arg("y1"),
            py::arg("x2"),
            py::arg("y2"),
            py::arg("p") = olc::WHITE,
            py::arg("pattern") = 0xFFFFFFFF)
        .def("DrawLine", py::overload_cast<const olc::vi2d&, const olc::vi2d&, olc::Pixel, uint32_t>(&PGE::DrawLine),
            "Draws a line from (x1,y1) to (x2,y2)\n",
            py::arg("pos1"),
            py::arg("pos2"),
            py::arg("p") = olc::WHITE,
            py::arg("pattern") = 0xFFFFFFFF)
        .def("DrawCircle", py::overload_cast<int32_t, int32_t, int32_t, olc::Pixel, uint8_t>(&PGE::DrawCircle),
            "Draws a circle located at (x,y) with radius\n",
            py::arg("x"),
            py::arg("y"),
            py::arg("radius"),
            py::arg("p") = olc::WHITE,
            py::arg("mask") = 0xFF)
        .def("DrawCircle", py::overload_cast<const olc::vi2d&, int32_t, olc::Pixel, uint8_t>(&PGE::DrawCircle),
            "Draws a circle located at (x,y) with radius\n",
            py::arg("pos"),
            py::arg("radius"),
            py::arg("p") = olc::WHITE,
            py::arg("mask") = 0xFF)
        .def("FillCircle", py::overload_cast<int32_t, int32_t, int32_t, olc::Pixel>(&PGE::FillCircle),
            "Fills a circle located at (x,y) with radius\n",
            py::arg("x"),
            py::arg("y"),
            py::arg("radius"),
            py::arg("p") = olc::WHITE)
        .def("FillCircle", py::overload_cast<const olc::vi2d&, int32_t, olc::Pixel>(&PGE::FillCircle),
            "Fills a circle located at (x,y) with radius\n",
            py::arg("pos"),
            py::arg("radius"),
            py::arg("p") = olc::WHITE)
        .def("DrawRect", py::overload_cast<int32_t, int32_t, int32_t, int32_t, olc::Pixel>(&PGE::DrawRect),
            "Draws a rectangle at (x,y) to (x+w,y+h)\n",
            py::arg("x"),
            py::arg("y"),
            py::arg("w"),
            py::arg("h"),
            py::arg("p") = olc::WHITE)
        .def("DrawRect", py::overload_cast<const olc::vi2d&, const olc::vi2d&, olc::Pixel>(&PGE::DrawRect),
            "Draws a rectangle at (x,y) to (x+w,y+h)\n",
            py::arg("pos"),
            py::arg("size"),
            py::arg("p") = olc::WHITE)
        .def("FillRect", py::overload_cast<int32_t, int32_t, int32_t, int32_t, olc::Pixel>(&PGE::FillRect),
            "Fills a rectangle at (x,y) to (x+w,y+h)\n",
            py::arg("x"),
            py::arg("y"),
            py::arg("w"),
            py::arg("h"),
            py::arg("p") = olc::WHITE)
        .def("FillRect", py::overload_cast<const olc::vi2d&, const olc::vi2d&, olc::Pixel>(&PGE::FillRect),
            "Fills a rectangle at (x,y) to (x+w,y+h)\n",
            py::arg("pos"),
            py::arg("size"),
            py::arg("p") = olc::WHITE)
        .def("DrawTriangle", py::overload_cast<int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, olc::Pixel>(&PGE::DrawTriangle),
            "Draws a triangle between points (x1,y1), (x2,y2) and (x3,y3)\n",
            py::arg("x1"),
            py::arg("y1"),
            py::arg("x2"),
            py::arg("y2"),
            py::arg("x3"),
            py::arg("y3"),
            py::arg("p") = olc::WHITE)
        .def("DrawTriangle", py::overload_cast<const olc::vi2d&, const olc::vi2d&, const olc::vi2d&, olc::Pixel>(&PGE::DrawTriangle),
            "Draws a triangle between points (x1,y1), (x2,y2) and (x3,y3)\n",
            py::arg("pos1"),
            py::arg("pos2"),
            py::arg("pos3"),
            py::arg("p") = olc::WHITE)
        .def("FillTriangle", py::overload_cast<int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, olc::Pixel>(&PGE::FillTriangle),
            "Flat fills a triangle between points (x1,y1), (x2,y2) and (x3,y3)\n",
            py::arg("x1"),
            py::arg("y1"),
            py::arg("x2"),
            py::arg("y2"),
            py::arg("x3"),
            py::arg("y3"),
            py::arg("p") = olc::WHITE)
        .def("FillTriangle", py::overload_cast<const olc::vi2d&, const olc::vi2d&, const olc::vi2d&, olc::Pixel>(&PGE::FillTriangle),
            "Flat fills a triangle between points (x1,y1), (x2,y2) and (x3,y3)\n",
            py::arg("pos1"),
            py::arg("pos2"),
            py::arg("pos3"),
            py::arg("p") = olc::WHITE)
        .def("DrawSprite", py::overload_cast<int32_t, int32_t, olc::Sprite*, uint32_t, uint8_t>(&PGE::DrawSprite),
            "Draws an entire sprite at location (x,y)\n",
            py::arg("x"),
            py::arg("y"),
            py::arg("sprite"),
            py::arg("scale") = 1,
            py::arg("flip") = olc::Sprite::NONE)
        .def("DrawSprite", py::overload_cast<const olc::vi2d&, olc::Sprite*, uint32_t, uint8_t>(&PGE::DrawSprite),
            "Draws an entire sprite at location (x,y)\n",
            py::arg("pos"),
            py::arg("sprite"),
            py::arg("scale") = 1,
            py::arg("flip") = olc::Sprite::NONE)
        .def("DrawPartialSprite", py::overload_cast<int32_t, int32_t, olc::Sprite*, int32_t, int32_t, int32_t, int32_t, uint32_t, uint8_t>(&PGE::DrawPartialSprite),
            "Draws an area of a sprite at location (x,y), where the\nselected area is (ox,oy) to (ox+w,oy+h)\n",
            py::arg("x"),
            py::arg("y"),
            py::arg("sprite"),
            py::arg("ox"),
            py::arg("oy"),
            py::arg("w"),
            py::arg("h"),
            py::arg("scale") = 1,
            py::arg("flip") = olc::Sprite::NONE)
        .def("DrawPartialSprite", py::overload_cast<const olc::vi2d&, olc::Sprite*, const olc::vi2d&, const olc::vi2d&, uint32_t, uint8_t>(&PGE::DrawPartialSprite),
            "Draws an area of a sprite at location (x,y), where the\nselected area is (ox,oy) to (ox+w,oy+h)\n",
            py::arg("pos"),
            py::arg("sprite"),
            py::arg("sourcepos"),
            py::arg("size"),
            py::arg("scale") = 1,
            py::arg("flip") = olc::Sprite::NONE)
        .def("SetDecalMode", &PGE::SetDecalMode,
            "Decal Quad functions\n",
            py::arg("mode"))
        .def("DrawDecal", &PGE::DrawDecal,
            "Draws a whole decal, with optional scale and tinting\n",
            py::arg("pos"),
            py::arg("decal"),
            py::arg("scale") = olc::vf2d(1.0f, 1.0f),
            py::arg("tint") = olc::WHITE)
        .def("DrawPartialDecal", py::overload_cast<const olc::vf2d&, olc::Decal*, const olc::vf2d&, const olc::vf2d&, const olc::vf2d&, const olc::Pixel&>(&PGE::DrawPartialDecal),
            "Draws a region of a decal, with optional scale and tinting\n",
            py::arg("pos"),
            py::arg("decal"),
            py::arg("source_pos"),
            py::arg("source_size"),
            py::arg("scale") = olc::vf2d(1.0f, 1.0f),
            py::arg("tint") = olc::WHITE)
        .def("DrawPartialDecal", py::overload_cast<const olc::vf2d&, const olc::vf2d&, olc::Decal*, const olc::vf2d&, const olc::vf2d&, const olc::Pixel&>(&PGE::DrawPartialDecal),
            "Draws a region of a decal, with optional scale and tinting\n",
            py::arg("pos"),
            py::arg("size"),
            py::arg("decal"),
            py::arg("source_pos"),
            py::arg("source_size"),
            py::arg("tint") = olc::WHITE)
        .def("DrawExplicitDecal", &PGE::DrawExplicitDecal,
            "Draws fully user controlled 4 vertices, pos(pixels), uv(pixels), colours\n",
            py::arg("decal"),
            py::arg("pos"),
            py::arg("uv"),
            py::arg("col"),
            py::arg("elements") = 4)
        /* It seems like pybind11 doesn't support abstract declaration yet */
        // .def("DrawWarpedDecal", py::overload_cast<olc::Decal*, const olc::vf2d(&)[4], const olc::Pixel&>(&PGE::DrawWarpedDecal),
        //     "Draws a decal with 4 arbitrary points, warping the texture to look \"correct\"\n",
        //     py::arg("decal"),
        //     py::arg("pos"),
        //     py::arg("tint") = olc::WHITE)
        .def("DrawWarpedDecal", py::overload_cast<olc::Decal*, const olc::vf2d*, const olc::Pixel&>(&PGE::DrawWarpedDecal),
            "Draws a decal with 4 arbitrary points, warping the texture to look \"correct\"\n",
            py::arg("decal"),
            py::arg("pos"),
            py::arg("tint") = olc::WHITE)
        .def("DrawWarpedDecal", py::overload_cast<olc::Decal*, const std::array<olc::vf2d, 4>&, const olc::Pixel&>(&PGE::DrawWarpedDecal),
            "Draws a decal with 4 arbitrary points, warping the texture to look \"correct\"\n",
            py::arg("decal"),
            py::arg("pos"),
            py::arg("tint") = olc::WHITE)
        // .def("DrawPartialWarpedDecal", py::overload_cast<olc::Decal*, const olc::vf2d(&)[4], const olc::vf2d&, const olc::vf2d&, const olc::Pixel&>(&PGE::DrawPartialWarpedDecal),
        //     "As above, but you can specify a region of a decal source sprite\n",
        //     py::arg("decal"),
        //     py::arg("pos"),
        //     py::arg("source_pos"),
        //     py::arg("source_size"),
        //     py::arg("tint") = olc::WHITE)
        .def("DrawPartialWarpedDecal", py::overload_cast<olc::Decal*, const olc::vf2d*, const olc::vf2d&, const olc::vf2d&, const olc::Pixel&>(&PGE::DrawPartialWarpedDecal),
            "As above, but you can specify a region of a decal source sprite\n",
            py::arg("decal"),
            py::arg("pos"),
            py::arg("source_pos"),
            py::arg("source_size"),
            py::arg("tint") = olc::WHITE)
        .def("DrawPartialWarpedDecal", py::overload_cast<olc::Decal*, const std::array<olc::vf2d, 4>&, const olc::vf2d&, const olc::vf2d&, const olc::Pixel&>(&PGE::DrawPartialWarpedDecal),
            "As above, but you can specify a region of a decal source sprite\n",
            py::arg("decal"),
            py::arg("pos"),
            py::arg("source_pos"),
            py::arg("source_size"),
            py::arg("tint") = olc::WHITE)
        .def("DrawRotatedDecal", &PGE::DrawRotatedDecal,
            "Draws a decal rotated to specified angle, wit point of rotation offset\n",
            py::arg("pos"),
            py::arg("decal"),
            py::arg("fAngle"),
            py::arg("center") = olc::vf2d( 0.0f, 0.0f ),
            py::arg("scale") = olc::vf2d( 1.0f, 1.0f ),
            py::arg("tint") = olc::WHITE)
        .def("DrawPartialRotatedDecal", &PGE::DrawPartialRotatedDecal,
            py::arg("pos"),
            py::arg("decal"),
            py::arg("fAngle"),
            py::arg("center"),
            py::arg("source_pos"),
            py::arg("source_size"),
            py::arg("scale") = olc::vf2d( 1.0f, 1.0f ),
            py::arg("tint") = olc::WHITE)
        .def("DrawStringDecal", &PGE::DrawStringDecal,
            "Draws a multiline string as a decal, with tiniting and scaling\n",
            py::arg("pos"),
            py::arg("sText"),
            py::arg("col") = olc::WHITE,
            py::arg("scale") = olc::vf2d( 1.0f, 1.0f ))
        .def("DrawStringPropDecal", &PGE::DrawStringPropDecal,
            py::arg("pos"),
            py::arg("sText"),
            py::arg("col") = olc::WHITE,
            py::arg("scale") = olc::vf2d( 1.0f, 1.0f ))
        .def("FillRectDecal", &PGE::FillRectDecal,
            "Draws a single shaded filled rectangle as a decal\n",
            py::arg("pos"),
            py::arg("size"),
            py::arg("col") = olc::WHITE)
        .def("GradientFillRectDecal", &PGE::GradientFillRectDecal,
            "Draws a corner shaded rectangle as a decal\n",
            py::arg("pos"),
            py::arg("size"),
            py::arg("colTL"),
            py::arg("colBL"),
            py::arg("colBR"),
            py::arg("colTR"))
        .def("DrawPolygonDecal", &PGE::DrawPolygonDecal,
            "Draws an arbitrary convex textured polygon using GPU\n",
            py::arg("decal"),
            py::arg("pos"),
            py::arg("uv"),
            py::arg("tint") = olc::WHITE)
        .def("DrawString", py::overload_cast<int32_t, int32_t, const std::string&, olc::Pixel, uint32_t>(&PGE::DrawString),
            "Draws a single line of text - traditional monospaced\n",
            py::arg("x"),
            py::arg("y"),
            py::arg("sText"),
            py::arg("col") = olc::WHITE,
            py::arg("scale") = 1)
        .def("DrawString", py::overload_cast<const olc::vi2d&, const std::string&, olc::Pixel, uint32_t>(&PGE::DrawString),
            "Draws a single line of text - traditional monospaced\n",
            py::arg("pos"),
            py::arg("sText"),
            py::arg("col") = olc::WHITE,
            py::arg("scale") = 1)
        .def("GetTextSize", &PGE::GetTextSize,
            py::arg("s"))
        .def("DrawStringProp", py::overload_cast<int32_t, int32_t, const std::string&, olc::Pixel, uint32_t>(&PGE::DrawStringProp),
            "Draws a single line of text - non-monospaced\n",
            py::arg("x"),
            py::arg("y"),
            py::arg("sText"),
            py::arg("col") = olc::WHITE,
            py::arg("scale") = 1)
        .def("DrawStringProp", py::overload_cast<const olc::vi2d&, const std::string&, olc::Pixel, uint32_t>(&PGE::DrawStringProp),
            "Draws a single line of text - non-monospaced\n",
            py::arg("pos"),
            py::arg("sText"),
            py::arg("col") = olc::WHITE,
            py::arg("scale") = 1)
        .def("GetTextSizeProp", &PGE::GetTextSizeProp,
            py::arg("s"))
        .def("Clear", &PGE::Clear,
            "Clears entire draw target to Pixel\n",
            py::arg("p"))
        .def("ClearBuffer", &PGE::ClearBuffer,
            "Clears the rendering back buffer\n",
            py::arg("p"),
            py::arg("bDepth") = true)
        .def("GetFontSprite", &PGE::GetFontSprite,
            "Returns the font image\n")
        .def("olc_UpdateMouse", &PGE::olc_UpdateMouse,
            py::arg("x"),
            py::arg("y"))
        .def("olc_UpdateMouseWheel", &PGE::olc_UpdateMouseWheel,
            py::arg("delta"))
        .def("olc_UpdateWindowSize", &PGE::olc_UpdateWindowSize,
            py::arg("x"),
            py::arg("y"))
        .def("olc_UpdateViewport", &PGE::olc_UpdateViewport)
        .def("olc_ConstructFontSheet", &PGE::olc_ConstructFontSheet)
        .def("olc_CoreUpdate", &PGE::olc_CoreUpdate)
        .def("olc_PrepareEngine", &PGE::olc_PrepareEngine)
        .def("olc_UpdateMouseState", &PGE::olc_UpdateMouseState,
            py::arg("button"),
            py::arg("state"))
        .def("olc_UpdateKeyState", &PGE::olc_UpdateKeyState,
            py::arg("key"),
            py::arg("state"))
        .def("olc_UpdateMouseFocus", &PGE::olc_UpdateMouseFocus,
            py::arg("state"))
        .def("olc_UpdateKeyFocus", &PGE::olc_UpdateKeyFocus,
            py::arg("state"))
        .def("olc_Terminate", &PGE::olc_Terminate);

};

void def_rcode(py::module& m)
{
    py::enum_<olc::rcode>(m, "rcode")
        .value("FAIL", olc::rcode::FAIL)
        .value("OK", olc::rcode::OK)
        .value("NO_FILE", olc::rcode::NO_FILE)
        .export_values();
};

void def_Pixel(py::module& m)
{
    py::class_<olc::Pixel> pixel(m, "Pixel");
    // Constructor
    pixel.def(py::init<uint8_t, uint8_t, uint8_t, uint8_t>(),
              py::arg("red"),
              py::arg("green"),
              py::arg("blue"),
              py::arg("alpha") = olc::nDefaultAlpha)
         .def(py::init<uint32_t>(),
              py::arg("p"))
         .def(py::init<>());
    
    // Pixel value
    pixel.def_readwrite("r", &olc::Pixel::r)
         .def_readwrite("g", &olc::Pixel::g)
         .def_readwrite("b", &olc::Pixel::b)
         .def_readwrite("a", &olc::Pixel::a);
         // well nobody would use this anyway
         //.def_readwrite("n", &olc::Pixel::n)

    // enum Mode
    py::enum_<olc::Pixel::Mode>(pixel, "Mode")
        .value("NORMAL", olc::Pixel::Mode::NORMAL)
        .value("MASK", olc::Pixel::Mode::MASK)
        .value("ALPHA", olc::Pixel::Mode::ALPHA)
        .value("CUSTOM", olc::Pixel::Mode::CUSTOM)
        .export_values();

    // operators
    pixel.def(py::self == py::self)
         .def(py::self != py::self)
         .def(py::self * float())
         .def(py::self / float())
         .def(py::self *= float())
         .def(py::self /= float())
         .def(py::self + py::self)
         .def(py::self - py::self)
         .def(py::self += py::self)
         .def(py::self -= py::self)
         .def("inv", &olc::Pixel::inv);

    m.def("PixelF", &olc::PixelF,
        py::arg("red"),
        py::arg("green"),
        py::arg("blue"),
        py::arg("alpha") = 1.0f);
    m.def("PixelLerp", &olc::PixelLerp,
        py::arg("p1"),
        py::arg("p2"),
        py::arg("t"));

    // const Pixel
    m.attr("GREY") = olc::GREY;
    m.attr("DARK_GREY") = olc::DARK_GREY;
    m.attr("VERY_DARK_GREY") = olc::VERY_DARK_GREY;
    m.attr("RED") = olc::RED;
    m.attr("DARK_RED") = olc::DARK_RED;
    m.attr("VERY_DARK_RED") = olc::VERY_DARK_RED;
    m.attr("YELLOW") = olc::YELLOW;
    m.attr("DARK_YELLOW") = olc::DARK_YELLOW;
    m.attr("VERY_DARK_YELLOW") = olc::VERY_DARK_YELLOW;
    m.attr("GREEN") = olc::GREEN;
    m.attr("DARK_GREEN") = olc::DARK_GREEN;
    m.attr("VERY_DARK_GREEN") = olc::VERY_DARK_GREEN;
    m.attr("CYAN") = olc::CYAN;
    m.attr("DARK_CYAN") = olc::DARK_CYAN;
    m.attr("VERY_DARK_CYAN") = olc::VERY_DARK_CYAN;
    m.attr("BLUE") = olc::BLUE;
    m.attr("DARK_BLUE") = olc::DARK_BLUE;
    m.attr("VERY_DARK_BLUE") = olc::VERY_DARK_BLUE;
    m.attr("MAGENTA") = olc::MAGENTA;
    m.attr("DARK_MAGENTA") = olc::DARK_MAGENTA;
    m.attr("VERY_DARK_MAGENTA") = olc::VERY_DARK_MAGENTA;
    m.attr("WHITE") = olc::WHITE;
    m.attr("BLACK") = olc::BLACK;
    m.attr("BLANK") = olc::BLANK;
};

void def_Key(py::module& m)
{
    py::enum_<olc::Key>(m, "Key")
        .value("NONE", olc::Key::NONE)
        .value("A", olc::Key::A)
        .value("B", olc::Key::B)
        .value("C", olc::Key::C)
        .value("D", olc::Key::D)
        .value("E", olc::Key::E)
        .value("F", olc::Key::F)
        .value("G", olc::Key::G)
        .value("H", olc::Key::H)
        .value("I", olc::Key::I)
        .value("J", olc::Key::J)
        .value("K", olc::Key::K)
        .value("L", olc::Key::L)
        .value("M", olc::Key::M)
        .value("N", olc::Key::N)
        .value("O", olc::Key::O)
        .value("P", olc::Key::P)
        .value("Q", olc::Key::Q)
        .value("R", olc::Key::R)
        .value("S", olc::Key::S)
        .value("T", olc::Key::T)
        .value("U", olc::Key::U)
        .value("V", olc::Key::V)
        .value("W", olc::Key::W)
        .value("X", olc::Key::X)
        .value("Y", olc::Key::Y)
        .value("Z", olc::Key::Z)
        .value("K0", olc::Key::K0)
        .value("K1", olc::Key::K1)
        .value("K2", olc::Key::K2)
        .value("K3", olc::Key::K3)
        .value("K4", olc::Key::K4)
        .value("K5", olc::Key::K5)
        .value("K6", olc::Key::K6)
        .value("K7", olc::Key::K7)
        .value("K8", olc::Key::K8)
        .value("K9", olc::Key::K9)
        .value("F1", olc::Key::F1)
        .value("F2", olc::Key::F2)
        .value("F3", olc::Key::F3)
        .value("F4", olc::Key::F4)
        .value("F5", olc::Key::F5)
        .value("F6", olc::Key::F6)
        .value("F7", olc::Key::F7)
        .value("F8", olc::Key::F8)
        .value("F9", olc::Key::F9)
        .value("F10", olc::Key::F10)
        .value("F11", olc::Key::F11)
        .value("F12", olc::Key::F12)
        .value("UP", olc::Key::UP)
        .value("DOWN", olc::Key::DOWN)
        .value("LEFT", olc::Key::LEFT)
        .value("RIGHT", olc::Key::RIGHT)
        .value("SPACE", olc::Key::SPACE)
        .value("TAB", olc::Key::TAB)
        .value("SHIFT", olc::Key::SHIFT)
        .value("CTRL", olc::Key::CTRL)
        .value("INS", olc::Key::INS)
        .value("DEL", olc::Key::DEL)
        .value("HOME", olc::Key::HOME)
        .value("END", olc::Key::END)
        .value("PGUP", olc::Key::PGUP)
        .value("PGDN", olc::Key::PGDN)
        .value("BACK", olc::Key::BACK)
        .value("ESCAPE", olc::Key::ESCAPE)
        .value("RETURN", olc::Key::RETURN)
        .value("ENTER", olc::Key::ENTER)
        .value("PAUSE", olc::Key::PAUSE)
        .value("SCROLL", olc::Key::SCROLL)
        .value("NP0", olc::Key::NP0)
        .value("NP1", olc::Key::NP1)
        .value("NP2", olc::Key::NP2)
        .value("NP3", olc::Key::NP3)
        .value("NP4", olc::Key::NP4)
        .value("NP5", olc::Key::NP5)
        .value("NP6", olc::Key::NP6)
        .value("NP7", olc::Key::NP7)
        .value("NP8", olc::Key::NP8)
        .value("NP9", olc::Key::NP9)
        .value("NP_MUL", olc::Key::NP_MUL)
        .value("NP_DIV", olc::Key::NP_DIV)
        .value("NP_ADD", olc::Key::NP_ADD)
        .value("NP_SUB", olc::Key::NP_SUB)
        .value("NP_DECIMAL", olc::Key::NP_DECIMAL)
        .value("PERIOD", olc::Key::PERIOD)
        .value("EQUALS", olc::Key::EQUALS)
        .value("COMMA", olc::Key::COMMA)
        .value("MINUS", olc::Key::MINUS)
        .value("OEM_1", olc::Key::OEM_1)
        .value("OEM_2", olc::Key::OEM_2)
        .value("OEM_3", olc::Key::OEM_3)
        .value("OEM_4", olc::Key::OEM_4)
        .value("OEM_5", olc::Key::OEM_5)
        .value("OEM_6", olc::Key::OEM_6)
        .value("OEM_7", olc::Key::OEM_7)
        .value("OEM_8", olc::Key::OEM_8)
        .value("CAPS_LOCK", olc::Key::CAPS_LOCK)
        .value("ENUM_END", olc::Key::ENUM_END)
        .export_values();
};

template<class T>
void def_vX2d(py::module& m, const std::string & name)
{
    py::class_<olc::v2d_generic<T>> vx(m, name.c_str());
    vx.def(py::init<>())
      .def(py::init<T, T>(),
           py::arg("_x"),
           py::arg("_y"))
      .def(py::init<const olc::v2d_generic<T>&>(),
           py::arg("v"))
      .def(py::init<olc::v2d_generic<int32_t>>())
      .def(py::init<olc::v2d_generic<float>>())
      .def(py::init<olc::v2d_generic<double>>())
      .def("mag", &olc::v2d_generic<T>::mag)
      .def("mag2", &olc::v2d_generic<T>::mag2)
      .def("norm", &olc::v2d_generic<T>::norm)
      .def("perp", &olc::v2d_generic<T>::perp)
      .def("floor", &olc::v2d_generic<T>::floor)
      .def("ceil", &olc::v2d_generic<T>::ceil)
      .def("max", &olc::v2d_generic<T>::max,
           py::arg("v"))
      .def("min", &olc::v2d_generic<T>::min,
           py::arg("v"))
      .def("dot", &olc::v2d_generic<T>::dot,
           py::arg("rhs"))
      .def("cross", &olc::v2d_generic<T>::cross,
           py::arg("rhs"))
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def(py::self * T())
      .def(py::self / T())
      .def(py::self *= T())
      .def(py::self /= T())
      .def(float() * py::self)
      .def(double() * py::self)
      .def(int() * py::self)
      .def(float() / py::self)
      .def(double() / py::self)
      .def(int() / py::self)
      .def(py::self < olc::v2d_generic<int32_t>())
      .def(py::self < olc::v2d_generic<float>())
      .def(py::self < olc::v2d_generic<double>())
      .def(py::self > olc::v2d_generic<int32_t>())
      .def(py::self > olc::v2d_generic<float>())
      .def(py::self > olc::v2d_generic<double>())
      .def(py::self + py::self)
      .def(py::self - py::self)
      .def(py::self * py::self)
      .def(py::self / py::self)
      .def(py::self += py::self)
      .def(py::self -= py::self)
      .def(py::self *= py::self)
      .def(py::self /= py::self)
      .def(+py::self)
      .def(-py::self)
      .def("__repr__", &olc::v2d_generic<T>::str)
      .def("str", &olc::v2d_generic<T>::str)
      .def_readwrite("x", &olc::v2d_generic<T>::x)
      .def_readwrite("y", &olc::v2d_generic<T>::y);
}

void def_HWButton(py::module& m)
{
    py::class_<olc::HWButton>(m, "HWButton")
        .def(py::init<>())
        .def_readwrite("bPressed", &olc::HWButton::bPressed)
        .def_readwrite("bReleased", &olc::HWButton::bReleased)
        .def_readwrite("bHeld", &olc::HWButton::bHeld);
}

void def_ResourceBuffer(py::module& m)
{
    py::class_<olc::ResourceBuffer>(m, "ResourceBuffer")
        .def(py::init<std::ifstream&, uint32_t, uint32_t>(),
             py::arg("ifs"),
             py::arg("offset"),
             py::arg("size"))
        .def_readwrite("vMemory", &olc::ResourceBuffer::vMemory);
}

void def_ResourcePack(py::module& m)
{
    py::class_<olc::ResourcePack>(m, "ResourcePack")
        .def(py::init<>())
        .def("AddFile", &olc::ResourcePack::AddFile,
             py::arg("sFile"))
        .def("LoadFile", &olc::ResourcePack::LoadPack,
             py::arg("sFile"),
             py::arg("sKey"))
        .def("SavePack", &olc::ResourcePack::SavePack,
             py::arg("sFile"),
             py::arg("sKey"))
        .def("GetFileBuffer", &olc::ResourcePack::GetFileBuffer,
             py::arg("sFile"))
        .def("Loaded", &olc::ResourcePack::Loaded);
}

void def_Sprite(py::module& m)
{
    py::class_<olc::Sprite> sprite(m, "Sprite");
    sprite.def(py::init<>())
          .def(py::init<const std::string&, olc::ResourcePack*>(),
              py::arg("sImageFile"),
              py::arg("pack") = nullptr)
          .def(py::init<int32_t, int32_t>(),
              py::arg("w"),
              py::arg("h"))
          .def("LoadFromFile", &olc::Sprite::LoadFromFile,
              py::arg("sImageFile"),
              py::arg("pack") = nullptr)
          .def("LoadFromPGESprFile", &olc::Sprite::LoadFromPGESprFile,
              py::arg("sImageFile"),
              py::arg("pack") = nullptr)
          .def("SaveToPGESprFile", &olc::Sprite::SaveToPGESprFile,
              py::arg("sImageFile"))
          .def_readwrite("width", &olc::Sprite::width)
          .def_readwrite("height", &olc::Sprite::width);

    py::enum_<olc::Sprite::Mode>(sprite, "Mode")
        .value("NORMAL", olc::Sprite::Mode::NORMAL)
        .value("PERIODIC", olc::Sprite::Mode::PERIODIC)
        .export_values();

    py::enum_<olc::Sprite::Flip>(sprite, "Flip")
        .value("NONE", olc::Sprite::Flip::NONE)
        .value("HORIZ", olc::Sprite::Flip::HORIZ)
        .value("VERT", olc::Sprite::Flip::VERT)
        .export_values();

    sprite.def("SetSampleMode", &olc::Sprite::SetSampleMode,
              py::arg("mode") = olc::Sprite::Mode::NORMAL)
        .def("GetPixel", py::overload_cast<int32_t, int32_t>(&olc::Sprite::GetPixel, py::const_),
            py::arg("x"),
            py::arg("y"))
        .def("SetPixel", py::overload_cast<int32_t, int32_t, olc::Pixel>(&olc::Sprite::SetPixel),
            py::arg("x"),
            py::arg("y"),
            py::arg("p"))
        .def("GetPixel", py::overload_cast<const olc::vi2d&>(&olc::Sprite::GetPixel, py::const_),
            py::arg("a"))
        .def("SetPixel", py::overload_cast<const olc::vi2d&, olc::Pixel>(&olc::Sprite::SetPixel),
            py::arg("a"),
            py::arg("p"))
        .def("Sample", &olc::Sprite::Sample,
            py::arg("x"),
            py::arg("y"))
        .def("SampleBL", &olc::Sprite::SampleBL,
            py::arg("u"),
            py::arg("v"))
        .def("GetData", &olc::Sprite::GetData)
        .def("Duplicate", py::overload_cast<>(&olc::Sprite::Duplicate))
        .def("Duplicate",
            py::overload_cast<const olc::vi2d&, const olc::vi2d&>(&olc::Sprite::Duplicate),
            py::arg("vPos"),
            py::arg("vSize"))
        .def_readwrite("pColData", &olc::Sprite::pColData)
        .def_readwrite("modeSample", &olc::Sprite::modeSample);
}

void def_Decal(py::module& m)
{
    py::class_<olc::Decal>(m, "Decal")
        .def(py::init<olc::Sprite*, bool>(),
             py::arg("spr"),
             py::arg("filter") = false)
        .def(py::init<const uint32_t, olc::Sprite*>(),
             py::arg("nExistingTextureResource"),
             py::arg("spr"))
        .def("Update", &olc::Decal::Update)
        .def("UpdateSprite", &olc::Decal::UpdateSprite)
        .def_readwrite("id", &olc::Decal::id)
        .def_readwrite("sprite", &olc::Decal::sprite)
        .def_readwrite("vUVScale", &olc::Decal::vUVScale);
    
    py::enum_<olc::DecalMode>(m, "DecalMode")
        .value("NORMAL", olc::DecalMode::NORMAL)
        .value("ADDITIVE", olc::DecalMode::ADDITIVE)
        .value("MULTIPLICATIVE", olc::DecalMode::MULTIPLICATIVE)
        .value("STENCIL", olc::DecalMode::STENCIL)
        .value("ILLUMINATE", olc::DecalMode::ILLUMINATE)
        .value("WIREFRAME", olc::DecalMode::WIREFRAME)
        .export_values();
}

void def_Renderable(py::module& m)
{
    py::class_<olc::Renderable>(m, "Renderable")
        .def(py::init<>())
        .def("Load", &olc::Renderable::Load,
            py::arg("sFile"),
            py::arg("pack") = nullptr,
            py::arg("filter") = false)
        .def("Create", &olc::Renderable::Create,
            py::arg("width"),
            py::arg("height"),
            py::arg("filter") = false)
        .def("Decal", &olc::Renderable::Decal)
        .def("Sprite", &olc::Renderable::Sprite);
}

PYBIND11_MODULE(PyPGE, m) {
    def_rcode(m);
    def_Pixel(m);
    def_Key(m);

    def_vX2d<int32_t>(m, "vi2d");
    def_vX2d<float>(m, "vf2d");
    def_vX2d<double>(m, "vd2d");
    /* We don't export olc::vu2d here, since there's no refer to this type at all
    in olcPixelGameEngine.h other than the typedef itself */
    // def_vX2d<uint32_t>(m, "vu2d");

    def_HWButton(m);
    def_ResourceBuffer(m);
    def_ResourcePack(m);
    def_Sprite(m);
    def_Decal(m);
    def_Renderable(m);

    def_PGE(m);

    m.doc() = R"pbdoc(
        OneLoneCoder PixelGameEngine Python Wrapper
        -----------------------

        .. currentmodule:: PyPGE
    )pbdoc";

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
