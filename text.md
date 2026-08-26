像素坐标系：

```cpp
float w=1280.0f;
float h=800.0f;
glm::mat4 proj=glm::ortho(0.0f,w,h,0.0f,-1.0f,1.0f);

glm::mat4 view=glm::mat4(1.0f);

painter.setProjectionMatrix(proj);
painter.setViewMatrix(view);

painter.drawRect({100,100},{200,150},{1.0f,0.0f,0.0f,1.0f});
```

NDC坐标系：

```cpp
glm::mat4 proj=glm::ortho(-1.0f,1.0f,-1.0f,1.0f,-1.0f,1.0f);

glm::mat4 view=glm::mat4(1.0f);

painter.setProjectionMatrix(proj);
painter.setViewMatrix(view);

painter.drawCircle({0.0f,0.0f},0.5f,{0.0f,0.0f,1.0f,1.0f});
```

像素坐标系 with camera

```cpp
float w=1280.0f,h=800.0f;
glm::mat4 proj=glm::ortho(0.0f,w,h,0.0f,-1.0f,1.0f);

glm::vec2 cameraPos={300.0f,200.0f};
glm::mat4 view=glm::translate(glm::mat4(1.0f),glm::vec3(-cameraPos.x,-cameraPos.y,0.0f));

painter.setProjectionMatrix(proj);
painter.setViewMatrix(view);

painter.drawRect({500,400},{100,100},{1.0f,0.0f,0.0f,1.0f});
```
