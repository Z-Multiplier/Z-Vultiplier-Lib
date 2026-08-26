# Z-Vultiplier Lib 引导

## 快速开始 | Quick Start

下面是一个色相三角形的例子：\
Here is an example of a hue triangle:

```cpp
#include "Z-Vultiplier.hpp"

int main(){
    Core::Initializer init("Quick Start");//Initializer.
    auto& manager=Window::WindowManager::instance();//Manager instance.
    auto window=manager.create(init,800,800,"Title");//w&h

    //Setup stage done.

    window->setRenderCallback([&](Render::Painter& painter){
        //Render callback function
        //1. Setup projection & view matrix
        glm::mat4 proj=glm::ortho(-1.0f,1.0f,-1.0f,1.0f,-1.0f,1.0f);//NDC
        glm::mat4 view=glm::mat4(1.0f);
        painter.setProjectionMatrix(proj);
        painter.setViewMatrix(view);

        //2. Drawing
        painter.resetTransform();//Remember this!
        painter.drawTriangle({-0.6f,-0.5f},
                             {0.0f, 0.6f},
                             {0.6f,-0.5f},
                             {1.0f,0.0f,0.0f,1.0f},
                             {0.0f,1.0f,0.0f,1.0f},
                             {0.0f,0.0f,1.0f,1.0f});
        //pos1,pos2,pos3,color1,color2,color3
        //Either Core::Color & glm::vec4 is avaliable
    });
    //Window setting done.

    Core::Clock clock([&](){
        return;
    },60);//loop,FPS

    while(clock){
        clock.run();
    }
    //done!
    return 0;
}
```

## 1. 窗口创建与管理 | Window Creating & Managing

在本项目中，创建窗口需要4个参数：初始化器，宽度，高度，标题\
In this project, creating a window needs 4 parameters: initializer, width, height, title

#### 1.1 初始化器 | Initializer

本项目中的初始化器只需要传入一个参数：应用名称\
Initializer in this project only needs one parameter: the application name

例如我们上文中提到的：\
For example, the one we mentioned earlier:

```cpp
Core::Initializer init("Quick Start");//Initializer.
```

"Quick Start"即为应用名\
"Quick Start" is the application name

#### 1.2 窗口管理器 | Window Manager

本项目实现了窗口管理器类，大部分窗口操作可以由其直接或间接完成\
This project has realized a 'window manager' class, most of the operations of the windows could be done by it (directly or indirectly)

窗口管理器采用单例模式，可以通过`Window::WindowManager::instance()`获取单例\
The window manager adopts singleton pattern, the instance could be got through `Window::WindowManager::instance()`

```cpp
auto& manager=Window::WindowManager::instance();
```

#### 1.3 窗口的创建 | Creating Window

正如上文所述，创建窗口需要4个参数：初始化器，宽度，高度，标题：\
As we mentioned earlier, creating a window needs 4 parameters: initializer, width, height and title:

---

- 初始化器：窗口的初始化器，提供`VkDevice`等`Vulkan`资源
- 宽度、高度：字面意思
- 标题：窗口的标题，类型为`std::string`

---

- initializer: Initializer of the window, provides `Vulkan` resources like `VkDevice`
- width & height: Literally
- title: title of the window, the type of it is `std::string`

---

```cpp
auto window=manager.create(init,800,800,"Title");
```

#### 1.4 窗口的关闭 | Closing Window

关闭窗口有两种方法：\
There's two ways to close a window:

- `window->close()`
- `manager.destroy(window)`

## 2. 基本渲染 | Basic Rendering

#### 2.1 设置渲染回调 | Setting Rendering Callback

设置窗口渲染回调只需要持有`Handle`指针即可：\
To set the window rendering callback, you only need to hold the `Handle` pointer: 

```cpp
window->setRenderCallback(...)
```

建议使用`lambda`表达式\
`lambda` function recommended

参数提供`Render::Painter`\
We provides `Render::Painter` in parameters

#### 2.2 渲染 | Rendering

本项目的渲染分为两步：\
Rendering in this project is separated into 2 parts: 

##### 2.2.a 投影与视口矩阵 | Projection & View Matrix

这将决定下文中使用的坐标系\
This decides the coordinate we'll use later

选择1：NDC坐标\
Choice 1: NDC Coordinate

```cpp
glm::mat4 proj=glm::ortho(-1.0f,1.0f,-1.0f,1.0f,-1.0f,1.0f);

glm::mat4 view=glm::mat4(1.0f);

painter.setProjectionMatrix(proj);
painter.setViewMatrix(view);
```

最终坐标范围为$[-1.0,1.0]$\
The final position range is $[-1.0,1.0]$

选择2：像素坐标\
Choice 2: Pixel Coordinate

```cpp
glm::mat4 proj=glm::ortho(0.0f,width,height,0.0f,-1.0f,1.0f);

glm::mat4 view=glm::mat4(1.0f);

painter.setProjectionMatrix(proj);
painter.setViewMatrix(view);
```

这是更加符合直觉的坐标系，直接使用像素坐标即可\
This is a more intuitive coordinate system, just use pixel coordinates directly

##### 2.2.b 图形的绘制 | Drawing

本项目实现了一系列基本渲染函数，下面将一一列出\
This project has realized a series of basic rendering functions, here's all of it

```cpp
void drawLine(vec2 p1,vec2 p2,vec4 color,float width=0.001f);
void drawPoint(vec2 p,vec4 color,float size=0.001f);
void drawTriangle(vec2 p1,vec2 p2,vec2 p3,vec4 color);
void drawTriangle(vec2 p1,vec2 p2,vec2 p3,vec4 c1,vec4 c2,vec4 c3);
void drawRect(vec2 pos,vec2 size,vec4 color);
void drawPolygon(const std::vector<vec2>& vertices,const vec4& color);
void drawCircle(vec2 center,float radius,const vec4& color,int segments=32);
void drawRoundedRect(vec2 pos,vec2 size,float radius,vec4 color);
void drawEllipse(vec2 center,vec2 radius,vec4 color,int segments=32);
void drawRegularPolygon(vec2 center,float radius,int sides,vec4 color);
void drawStar(vec2 center,float outerRadius,float innerRadius,int points,vec4 color);
void drawPie(vec2 center,float radius,float startAngle,float endAngle,vec4 color,int segments=32);
void drawBezier(const std::vector<vec2>& controlPoints,vec4 color,int segments,float width=0.001f);
void putImage(vec2 pos,vec2 size,Image& img,vec4 tint={1,1,1,1});
void drawText(const Font& font,const std::string& text,vec2 pos,vec4 color={1,1,1,1},float scale=1.0f);
```

函数名和参数名已揭示函数用法，此处不再赘述\
The name of the function and the parameters has revealed the usage of the function, not repeat them here

对于每一个整体图形块，可参照下面的模式\
For each of the component of shapes, you can follow the pattern below

```cpp
//Setting transform matrix
painter.resetTransform()//Remember this!
painter.drawLine(...)//e.g.
```

#### 2.3 变换矩阵 | Transform Matrix

在`Matrix.hpp`中有如下工厂函数：\
These factory functions exists in `Matrix.hpp`

```cpp
inline glm::mat4 translate(float x, float y) {
    return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
}
inline glm::mat4 rotate(float angle) {
    return glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));
}
inline glm::mat4 scale(float sx, float sy) {
    return glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, 1.0f));
}
```

可直接使用工厂函数创建对应变换矩阵\
Could just use the factory functions to create the transform matrix

## 3. 加载资源 | Loading Resources

#### 3.1 加载图片 | Loading Images

只需使用`Painter::createImage(...)`即可\
Just use `Painter::createImage(...)`

它接受一个`std::string`作为文件路径，使用`auto`承接即可\
It accepts a `std::string` as file path, you could just use `auto` to handle it

#### 3.2 加载字体 | Loading Fonts

同上，使用`Painter::loadFont`即可\
Same as above, just use `Painter::loadFont`

## 4. 音频管理 | Audio Managing

#### 4.1 启动音频引擎 | Startup Audio Engine

与`WindowManager`一样，`Audio::Engine`同样采用单例模式\
Same as `WindowManager`, `Audio::Engine` adopts singleton pattern

直接获取即可：\
Get it directly:

```cpp
auto& engine=Audio::Engine::instance();
```

#### 4.2 创建声音对象 | Create Sound Object

在正式播放声音之前，我们需要加载一个声音\
Before we play the sound,we need to load a sound

`Audio::Sound`支持三种初始化方式：
`Audio::Sound` provides three ways to init:

```cpp
Sound(std::string filePath);
Sound(WaveForm wave,float hz,float amplitude,long double duration);
Sound(NoiseColor noise,float amplitude,long double duration);
```

其中：
Among them:

```cpp
enum class WaveForm{
    SINE,
    SQUARE,
    TRIANGLE,
    SAWTOOTH
};
enum class NoiseColor{
    WHITE,
    PINK,
    BROWN
};
```

#### 4.3 播放 | Play

直接将`Sound`对象`std::move`进`Audio::Engine::playSound()`即可\
Just `std::move` the `Sound` object into `Audio::Engine::playSound()`

`Clock`将会每帧`update`并清理结束的声音
`Clock` would `update` and remove the sound that already ends every single frame.

## 5. 杂物 | Rest

参考`API Reference`即可\
Refer to `API Reference`
